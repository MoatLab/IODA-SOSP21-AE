#include "qemu/osdep.h"
#include "hw/block/block.h"
#include "hw/pci/msix.h"
#include "hw/pci/msi.h"
#include "../nvme.h"
#include "ftl.h"

uint16_t ssd_id_cnt = 0;
struct ssd *ssd_array[4];
bool harmonia_override[4] = {false, false, false, false};
pthread_mutex_t harmonia_override_lock;

static void *ftl_thread(void *arg);

//unsigned int pages_read = 0;
int free_line_print_time[4] = {-1, -1, -1, -1};
int prev_time_s[4] = {0, 0, 0, 0};
uint64_t global_gc_endtime = 0;
uint64_t gc_endtime_array[4];
pthread_mutex_t global_gc_endtime_lock;
uint64_t prev_req_stimes[4];

static inline bool should_gc(struct ssd *ssd)
{
    return (ssd->lm.free_line_cnt <= ssd->sp.gc_thres_lines);
}

static inline bool should_gc_high(struct ssd *ssd)
{
    return (ssd->lm.free_line_cnt <= ssd->sp.gc_thres_lines_high);
}

static inline struct ppa get_maptbl_ent(struct ssd *ssd, uint64_t lpn)
{
    return ssd->maptbl[lpn];
}

static inline void set_maptbl_ent(struct ssd *ssd, uint64_t lpn, struct ppa *ppa)
{
    assert(lpn < ssd->sp.tt_pgs);
    ssd->maptbl[lpn] = *ppa;
}

static uint64_t ppa2pgidx(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    uint64_t pgidx;

    pgidx = ppa->g.ch * spp->pgs_per_ch + ppa->g.lun * spp->pgs_per_lun +
        ppa->g.pl * spp->pgs_per_pl + ppa->g.blk * spp->pgs_per_blk + ppa->g.pg;

    assert(pgidx < spp->tt_pgs);

    return pgidx;
}

static inline uint64_t get_rmap_ent(struct ssd *ssd, struct ppa *ppa)
{
    uint64_t pgidx = ppa2pgidx(ssd, ppa);

    return ssd->rmap[pgidx];
}

/* set rmap[page_no(ppa)] -> lpn */
static inline void set_rmap_ent(struct ssd *ssd, uint64_t lpn, struct ppa *ppa)
{
    uint64_t pgidx = ppa2pgidx(ssd, ppa);

    ssd->rmap[pgidx] = lpn;
}

static void ssd_init_lines(struct ssd *ssd)
{
    int i;
    struct ssdparams *spp = &ssd->sp;
    struct line_mgmt *lm = &ssd->lm;
    struct line *line;

    lm->tt_lines = spp->blks_per_pl;
    assert(lm->tt_lines == spp->tt_lines);
    lm->lines = g_malloc0(sizeof(struct line) * lm->tt_lines);

    QTAILQ_INIT(&lm->free_line_list);
    QTAILQ_INIT(&lm->victim_line_list);
    QTAILQ_INIT(&lm->full_line_list);

    lm->free_line_cnt = 0;
    for (i = 0; i < lm->tt_lines; i++) {
        line = &lm->lines[i];
        line->id = i;
        line->ipc = 0;
        line->vpc = 0;
        /* initialize all the lines as free lines */
        QTAILQ_INSERT_TAIL(&lm->free_line_list, line, entry);
        lm->free_line_cnt++;
    }

    assert(lm->free_line_cnt == lm->tt_lines);
    lm->victim_line_cnt = 0;
    lm->full_line_cnt = 0;
}

static void ssd_init_write_pointer(struct ssd *ssd)
{
    struct write_pointer *wpp = &ssd->wp;
    struct line_mgmt *lm = &ssd->lm;
    struct line *curline = NULL;
    /* make sure lines are already initialized by now */
    curline = QTAILQ_FIRST(&lm->free_line_list);
    QTAILQ_REMOVE(&lm->free_line_list, curline, entry);
    lm->free_line_cnt--;
    /* wpp->curline is always our onging line for writes */
    wpp->curline = curline;
    wpp->ch = 0;
    wpp->lun = 0;
    wpp->pg = 0;
    wpp->blk = 0;
    wpp->pl = 0;
}

static inline void check_addr(int a, int max)
{
    assert(a >= 0 && a < max);
}

static struct line *get_next_free_line(struct ssd *ssd)
{
    struct line_mgmt *lm = &ssd->lm;
    struct line *curline = NULL;

    curline = QTAILQ_FIRST(&lm->free_line_list);
    if (!curline) {
        printf("FEMU-FTL: Error, there is no free lines left in [%s] !!!!\n", ssd->ssdname);
        return NULL;
    }

    QTAILQ_REMOVE(&lm->free_line_list, curline, entry);
    lm->free_line_cnt--;
    return curline;
}

static void ssd_advance_write_pointer(struct ssd *ssd)
{
    struct ssdparams *spp = &ssd->sp;
    struct write_pointer *wpp = &ssd->wp;
    struct line_mgmt *lm = &ssd->lm;

    check_addr(wpp->ch, spp->nchs);
    wpp->ch++;
    if (wpp->ch == spp->nchs) {
        wpp->ch = 0;
        check_addr(wpp->lun, spp->luns_per_ch);
        wpp->lun++;
        /* in this case, we should go to next lun */
        if (wpp->lun == spp->luns_per_ch) {
            wpp->lun = 0;
            /* go to next page in the block */
            check_addr(wpp->pg, spp->pgs_per_blk);
            wpp->pg++;
            if (wpp->pg == spp->pgs_per_blk) {
                wpp->pg = 0;
                /* move current line to {victim,full} line list */
                if (wpp->curline->vpc == spp->pgs_per_line) {
                    /* all pgs are still valid, move to full line list */
                    assert(wpp->curline->ipc == 0);
                    QTAILQ_INSERT_TAIL(&lm->full_line_list, wpp->curline, entry);
                    lm->full_line_cnt++;
                } else {
                    assert(wpp->curline->vpc >= 0 && wpp->curline->vpc < spp->pgs_per_line);
                    /* there must be some invalid pages in this line */
                    //printf("Coperd,curline,vpc:%d,ipc:%d\n", wpp->curline->vpc, wpp->curline->ipc);
                    assert(wpp->curline->ipc > 0);
                    QTAILQ_INSERT_TAIL(&lm->victim_line_list, wpp->curline, entry);
                    lm->victim_line_cnt++;
                }
                /* current line is used up, pick another empty line */
                check_addr(wpp->blk, spp->blks_per_pl);
                /* TODO: how should we choose the next block for writes */
                wpp->curline = NULL;
                wpp->curline = get_next_free_line(ssd);
                if (!wpp->curline) {
                    printf("ssd_advance_write_pointer calling abort\n");
                    abort();
                }
                wpp->blk = wpp->curline->id;
                check_addr(wpp->blk, spp->blks_per_pl);
                /* make sure we are starting from page 0 in the super block */
                assert(wpp->pg == 0);
                assert(wpp->lun == 0);
                assert(wpp->ch == 0);
                /* TODO: assume # of pl_per_lun is 1, fix later */
                assert(wpp->pl == 0);
            }
        }
    }
    //printf("Next,ch:%d,lun:%d,blk:%d,pg:%d\n", wpp->ch, wpp->lun, wpp->blk, wpp->pg);
}

static struct ppa get_new_page(struct ssd *ssd)
{
    struct write_pointer *wpp = &ssd->wp;
    struct ppa ppa;
    ppa.ppa = 0;
    ppa.g.ch = wpp->ch;
    ppa.g.lun = wpp->lun;
    ppa.g.pg = wpp->pg;
    ppa.g.blk = wpp->blk;
    ppa.g.pl = wpp->pl;
    assert(ppa.g.pl == 0);

    return ppa;
}

static void check_params(struct ssdparams *spp)
{
    /*
     * we are using a general write pointer increment method now, no need to
     * force luns_per_ch and nchs to be power of 2
     */

    //assert(is_power_of_2(spp->luns_per_ch));
    //assert(is_power_of_2(spp->nchs));
}

static void ssd_init_params(struct ssdparams *spp)
{
    spp->secsz = 512;
    spp->secs_per_pg = 8;
    spp->pgs_per_blk = 256;
    spp->blks_per_pl = 256; /* 16GB */
    spp->pls_per_lun = 1;
    spp->luns_per_ch = 8;
    spp->nchs = 8;

    spp->pg_rd_lat = 40000;
    spp->pg_wr_lat = 200000;
    spp->blk_er_lat = 2000000;
    spp->ch_xfer_lat = 0;

    /* calculated values */
    spp->secs_per_blk = spp->secs_per_pg * spp->pgs_per_blk;
    spp->secs_per_pl = spp->secs_per_blk * spp->blks_per_pl;
    spp->secs_per_lun = spp->secs_per_pl * spp->pls_per_lun;
    spp->secs_per_ch = spp->secs_per_lun * spp->luns_per_ch;
    spp->tt_secs = spp->secs_per_ch * spp->nchs;

    spp->pgs_per_pl = spp->pgs_per_blk * spp->blks_per_pl;
    spp->pgs_per_lun = spp->pgs_per_pl * spp->pls_per_lun;
    spp->pgs_per_ch = spp->pgs_per_lun * spp->luns_per_ch;
    spp->tt_pgs = spp->pgs_per_ch * spp->nchs;

    spp->blks_per_lun = spp->blks_per_pl * spp->pls_per_lun;
    spp->blks_per_ch = spp->blks_per_lun * spp->luns_per_ch;
    spp->tt_blks = spp->blks_per_ch * spp->nchs;

    spp->pls_per_ch =  spp->pls_per_lun * spp->luns_per_ch;
    spp->tt_pls = spp->pls_per_ch * spp->nchs;

    spp->tt_luns = spp->luns_per_ch * spp->nchs;

    /* line is special, put it at the end */
    spp->blks_per_line = spp->tt_luns; /* TODO: to fix under multiplanes */
    spp->pgs_per_line = spp->blks_per_line * spp->pgs_per_blk;
    spp->secs_per_line = spp->pgs_per_line * spp->secs_per_pg;
    spp->tt_lines = spp->blks_per_lun; /* TODO: to fix under multiplanes */

    spp->gc_thres_pcent = 0.90;
    spp->gc_thres_lines = (int)((1 - spp->gc_thres_pcent) * spp->tt_lines);
    spp->gc_thres_pcent_high = 0.95;
    spp->gc_thres_lines_high = (int)((1 - spp->gc_thres_pcent_high) * spp->tt_lines);
    //spp->gc_thres_lines_high = 1;
    spp->enable_gc_delay = true;
    spp->enable_gc_sync = false;
    spp->enable_free_blocks_log = false;
    spp->gc_sync_window = 100;
    spp->gc_sync_buffer = 50;
    spp->dynamic_gc_sync = false;
    spp->harmonia = false;

    printf("spp->pgs_per_line: %d\n", spp->pgs_per_line);
    printf("spp->tt_lines: %d\n", spp->tt_lines);
    printf("spp->tt_blks: %d\n", spp->tt_blks);
    printf("spp->gc_thres_lines: %d\n", spp->gc_thres_lines);

    check_params(spp);
}

static void ssd_init_nand_page(struct nand_page *pg, struct ssdparams *spp)
{
    int i;

    pg->nsecs = spp->secs_per_pg;
    pg->sec = g_malloc0(sizeof(nand_sec_status_t) * pg->nsecs);
    for (i = 0; i < pg->nsecs; i++) {
        pg->sec[i] = SEC_FREE;
    }
    pg->status = PG_FREE;
}

static void ssd_init_nand_blk(struct nand_block *blk, struct ssdparams *spp)
{
    int i;

    blk->npgs = spp->pgs_per_blk;
    blk->pg = g_malloc0(sizeof(struct nand_page) * blk->npgs);
    for (i = 0; i < blk->npgs; i++) {
        ssd_init_nand_page(&blk->pg[i], spp);
    }
    blk->ipc = 0;
    blk->vpc = 0;
    blk->erase_cnt = 0;
    blk->wp = 0;
}

static void ssd_init_nand_plane(struct nand_plane *pl, struct ssdparams *spp)
{
    int i;

    pl->nblks = spp->blks_per_pl;
    pl->blk = g_malloc0(sizeof(struct nand_block) * pl->nblks);
    for (i = 0; i < pl->nblks; i++) {
        ssd_init_nand_blk(&pl->blk[i], spp);
    }
}

static void ssd_init_nand_lun(struct nand_lun *lun, struct ssdparams *spp)
{
    int i;

    lun->npls = spp->pls_per_lun;
    lun->pl = g_malloc0(sizeof(struct nand_plane) * lun->npls);
    for (i = 0; i < lun->npls; i++) {
        ssd_init_nand_plane(&lun->pl[i], spp);
    }
    lun->next_lun_avail_time = 0;
    lun->busy = false;
    //lun->gc_counter = 0;
    //lun->gc_page_counter = 0;
}

static void ssd_init_ch(struct ssd_channel *ch, struct ssdparams *spp)
{
    int i;

    ch->nluns = spp->luns_per_ch;
    ch->lun = g_malloc0(sizeof(struct nand_lun) * ch->nluns);
    for (i = 0; i < ch->nluns; i++) {
        ssd_init_nand_lun(&ch->lun[i], spp);
    }
    ch->next_ch_avail_time = 0;
    ch->busy = 0;
}

static void ssd_init_maptbl(struct ssd *ssd)
{
    int i;
    struct ssdparams *spp = &ssd->sp;

    ssd->maptbl = g_malloc0(sizeof(struct ppa) * spp->tt_pgs);
    for (i = 0; i < spp->tt_pgs; i++) {
        ssd->maptbl[i].ppa = UNMAPPED_PPA;
    }
}

static void ssd_init_rmap(struct ssd *ssd)
{
    int i;
    struct ssdparams *spp = &ssd->sp;
    ssd->rmap = g_malloc0(sizeof(uint64_t) * spp->tt_pgs);
    for (i = 0; i < spp->tt_pgs; i++) {
        ssd->rmap[i] = INVALID_LPN;
    }
}

void ssd_init(struct ssd *ssd)
{
    int i;
    struct ssdparams *spp = &ssd->sp;

    assert(ssd);

    //pages_read = 0;
    /* assign ssd id for GC synchronization */
    ssd->id = ssd_id_cnt;
	ssd_array[ssd->id] = ssd;
    ssd_id_cnt++;
    printf("GCSYNC SSD initialized with id %d\n", ssd->id);
	pthread_mutex_init(&global_gc_endtime_lock, NULL);
	ssd->next_ssd_avail_time = 0;
	ssd->earliest_ssd_lun_avail_time = UINT64_MAX;
	gc_endtime_array[ssd->id] = 0;
	prev_req_stimes[ssd->id] = 0;
	pthread_mutex_init(&harmonia_override_lock, NULL);

	ssd->total_reads = 0;
	ssd->num_reads_blocked_by_gc[0] = 0;
	ssd->num_reads_blocked_by_gc[1] = 0;
	ssd->num_reads_blocked_by_gc[2] = 0;
	ssd->num_reads_blocked_by_gc[3] = 0;
	ssd->num_reads_blocked_by_gc[4] = 0;

    ssd_init_params(spp);

    /* initialize ssd internal layout architecture */
    ssd->ch = g_malloc0(sizeof(struct ssd_channel) * spp->nchs);
    for (i = 0; i < spp->nchs; i++) {
        ssd_init_ch(&ssd->ch[i], spp);
    }

    /* initialize maptbl */
    ssd_init_maptbl(ssd);

    /* initialize rmap */
    ssd_init_rmap(ssd);

    /* initialize all the lines */
    ssd_init_lines(ssd);

    /* initialize write pointer, this is how we allocate new pages for writes */
    ssd_init_write_pointer(ssd);

    qemu_thread_create(&ssd->ftl_thread, "ftl_thread", ftl_thread, ssd,
            QEMU_THREAD_JOINABLE);
}

static inline bool valid_ppa(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    int ch = ppa->g.ch;
    int lun = ppa->g.lun;
    int pl = ppa->g.pl;
    int blk = ppa->g.blk;
    int pg = ppa->g.pg;
    int sec = ppa->g.sec;

    if (ch >= 0 && ch < spp->nchs && lun >= 0 && lun < spp->luns_per_ch &&
            pl >= 0 && pl < spp->pls_per_lun && blk >= 0 &&
            blk < spp->blks_per_pl && pg >= 0 && pg < spp->pgs_per_blk &&
            sec >= 0 && sec < spp->secs_per_pg)
        return true;

    return false;
}

static inline bool valid_lpn(struct ssd *ssd, uint64_t lpn)
{
    return (lpn < ssd->sp.tt_pgs);
}

static inline bool mapped_ppa(struct ppa *ppa)
{
    return !(ppa->ppa == UNMAPPED_PPA);
}

static inline struct ssd_channel *get_ch(struct ssd *ssd, struct ppa *ppa)
{
    return &(ssd->ch[ppa->g.ch]);
}

static inline struct nand_lun *get_lun(struct ssd *ssd, struct ppa *ppa)
{
    struct ssd_channel *ch = get_ch(ssd, ppa);
    return &(ch->lun[ppa->g.lun]);
}

static inline struct nand_plane *get_pl(struct ssd *ssd, struct ppa *ppa)
{
    struct nand_lun *lun = get_lun(ssd, ppa);
    return &(lun->pl[ppa->g.pl]);
}

static inline struct nand_block *get_blk(struct ssd *ssd, struct ppa *ppa)
{
    struct nand_plane *pl = get_pl(ssd, ppa);
    return &(pl->blk[ppa->g.blk]);
}

static inline struct line *get_line(struct ssd *ssd, struct ppa *ppa)
{
    return &(ssd->lm.lines[ppa->g.blk]);
}

static inline struct nand_page *get_pg(struct ssd *ssd, struct ppa *ppa)
{
    struct nand_block *blk = get_blk(ssd, ppa);
    return &(blk->pg[ppa->g.pg]);
}

static uint64_t ssd_advance_status(struct ssd *ssd, struct ppa *ppa,
        struct nand_cmd *ncmd)
{
    int c = ncmd->cmd;
    uint64_t nand_stime;
    //uint64_t chnl_stime;
    struct ssdparams *spp = &ssd->sp;
    //struct ssd_channel *ch = get_ch(ssd, ppa);
    struct nand_lun *lun = get_lun(ssd, ppa);
    uint64_t cmd_stime = (ncmd->stime == 0) ? \
        qemu_clock_get_ns(QEMU_CLOCK_REALTIME) : ncmd->stime;
	// Why were we using current time? That doesn't seem to make sense as when to "start" a GC operation ever
    //uint64_t cmd_stime = (ncmd->stime == 0) ? \
        lun->next_lun_avail_time : ncmd->stime;
    uint64_t lat = 0;

    switch (c) {
    case NAND_READ:
        /* read: perform NAND cmd first */
        nand_stime = (lun->next_lun_avail_time < cmd_stime) ? cmd_stime : \
                     lun->next_lun_avail_time;
        lun->next_lun_avail_time = nand_stime + spp->pg_rd_lat;
        lat = lun->next_lun_avail_time - cmd_stime;
#if 0
        lun->next_lun_avail_time = nand_stime + spp->pg_rd_lat;

        /* read: then data transfer through channel */
        chnl_stime = (ch->next_ch_avail_time < lun->next_lun_avail_time) ? \
            lun->next_lun_avail_time : ch->next_ch_avail_time;
        ch->next_ch_avail_time = chnl_stime + spp->ch_xfer_lat;

        lat = ch->next_ch_avail_time - cmd_stime;
#endif
        break;

    case NAND_WRITE:
        /* write: transfer data through channel first */
        nand_stime = (lun->next_lun_avail_time < cmd_stime) ? cmd_stime : \
                     lun->next_lun_avail_time;
        lun->next_lun_avail_time = nand_stime + spp->pg_wr_lat;
        lat = lun->next_lun_avail_time - cmd_stime;

#if 0
        chnl_stime = (ch->next_ch_avail_time < cmd_stime) ? cmd_stime : \
                     ch->next_ch_avail_time;
        ch->next_ch_avail_time = chnl_stime + spp->ch_xfer_lat;

        /* write: then do NAND program */
        nand_stime = (lun->next_lun_avail_time < ch->next_ch_avail_time) ? \
            ch->next_ch_avail_time : lun->next_lun_avail_time;
        lun->next_lun_avail_time = nand_stime + spp->pg_wr_lat;

        lat = lun->next_lun_avail_time - cmd_stime;
#endif
        break;

    case NAND_ERASE:
        /* erase: only need to advance NAND status */

        nand_stime = (lun->next_lun_avail_time < cmd_stime) ? cmd_stime : \
                     lun->next_lun_avail_time;
        lun->next_lun_avail_time = nand_stime + spp->blk_er_lat;

        lat = lun->next_lun_avail_time - cmd_stime;
        break;

    default:
        printf("Unsupported NAND command: 0x%x\n", c);
    }

	if (lun->next_lun_avail_time > ssd->next_ssd_avail_time) {
		ssd->next_ssd_avail_time = lun->next_lun_avail_time;
	}
	if (lun->next_lun_avail_time < ssd->earliest_ssd_lun_avail_time) {
		ssd->earliest_ssd_lun_avail_time = lun->next_lun_avail_time;
	}

    return lat;
}

/* update SSD status about one page from PG_VALID -> PG_VALID */
static void mark_page_invalid(struct ssd *ssd, struct ppa *ppa)
{
    struct line_mgmt *lm = &ssd->lm;
    struct ssdparams *spp = &ssd->sp;
    struct nand_block *blk = NULL;
    struct nand_page *pg = NULL;
    bool was_full_line = false;
    struct line *line;

    /* update corresponding page status */
    pg = get_pg(ssd, ppa);
    assert(pg->status == PG_VALID);
    pg->status = PG_INVALID;

    /* update corresponding block status */
    blk = get_blk(ssd, ppa);
    assert(blk->ipc >= 0 && blk->ipc < spp->pgs_per_blk);
    blk->ipc++;
    assert(blk->vpc > 0 && blk->vpc <= spp->pgs_per_blk);
    blk->vpc--;

    /* update corresponding line status */
    line = get_line(ssd, ppa);
    assert(line->ipc >= 0 && line->ipc < spp->pgs_per_line);
    if (line->vpc == spp->pgs_per_line) {
        assert(line->ipc == 0);
        was_full_line = true;
    }
    line->ipc++;
    assert(line->vpc > 0 && line->vpc <= spp->pgs_per_line);
    line->vpc--;
    if (was_full_line) {
        /* move line: "full" -> "victim" */
        QTAILQ_REMOVE(&lm->full_line_list, line, entry);
        lm->full_line_cnt--;
        QTAILQ_INSERT_TAIL(&lm->victim_line_list, line, entry);
        lm->victim_line_cnt++;
    }
}

/* update SSD status about one page from PG_FREE -> PG_VALID */
static void mark_page_valid(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    struct nand_block *blk = NULL;
    struct nand_page *pg = NULL;
    struct line *line;

    /* update page status */
    pg = get_pg(ssd, ppa);
    assert(pg->status == PG_FREE);
    pg->status = PG_VALID;

    /* update corresponding block status */
    blk = get_blk(ssd, ppa);
    assert(blk->vpc >= 0 && blk->vpc < spp->pgs_per_blk);
    blk->vpc++;

    /* update corresponding line status */
    line = get_line(ssd, ppa);
    assert(line->vpc >= 0 && line->vpc < spp->pgs_per_line);
    line->vpc++;
}

/* only for erase, reset one block to free state */
static void mark_block_free(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    struct nand_block *blk = get_blk(ssd, ppa);
    struct nand_page *pg = NULL;
    int i;

    for (i = 0; i < spp->pgs_per_blk; i++) {
        /* reset page status */
        pg = &blk->pg[i];
        assert(pg->nsecs == spp->secs_per_pg);
        pg->status = PG_FREE;
    }

    /* reset block status */
    assert(blk->npgs == spp->pgs_per_blk);
    blk->ipc = 0;
    blk->vpc = 0;
    blk->erase_cnt++;
}

/* assume the read data will staged in DRAM and then flushed back to NAND */
static void gc_read_page(struct ssd *ssd, struct ppa *ppa)
{
    /* advance ssd status, we don't care about how long it takes */
    if (ssd->sp.enable_gc_delay) {
        struct nand_cmd gcr;
        gcr.cmd = NAND_READ;
        gcr.stime = 0;
        ssd_advance_status(ssd, ppa, &gcr);
    }
}

/* move valid page data (already in DRAM) from victim line to a new page */
static uint64_t gc_write_page(struct ssd *ssd, struct ppa *old_ppa)
{
    struct ppa new_ppa;
    struct ssd_channel *new_ch;
    struct nand_lun *new_lun;
    uint64_t lpn = get_rmap_ent(ssd, old_ppa);
    /* first read out current mapping info */
    //set_rmap(ssd, lpn, new_ppa);

    assert(valid_lpn(ssd, lpn));
    new_ppa = get_new_page(ssd);
    /* update maptbl */
    set_maptbl_ent(ssd, lpn, &new_ppa);
    /* update rmap */
    set_rmap_ent(ssd, lpn, &new_ppa);

    //mark_page_invalid(ssd, old_ppa);
    mark_page_valid(ssd, &new_ppa);

    /* need to advance the write pointer here */
    ssd_advance_write_pointer(ssd);

    if (ssd->sp.enable_gc_delay) {
        struct nand_cmd gcw;
        gcw.cmd = NAND_WRITE;
        gcw.stime = 0;
        ssd_advance_status(ssd, &new_ppa, &gcw);
    }

    /* advance per-ch gc_endtime as well */
    new_ch = get_ch(ssd, &new_ppa);
    new_ch->gc_endtime = new_ch->next_ch_avail_time;

    new_lun = get_lun(ssd, &new_ppa);
    new_lun->gc_endtime = new_lun->next_lun_avail_time;

    return 0;
}

/* TODO: now O(n) list traversing, optimize it later */
static struct line *select_victim_line(struct ssd *ssd, bool force)
{
    struct line_mgmt *lm = &ssd->lm;
    struct line *line, *victim_line = NULL;
    int max_ipc = 0;
    //int cnt = 0;

    if (QTAILQ_EMPTY(&lm->victim_line_list)) {
        //printf("QTAILQ_EMPTY(&lm->victim_line_list)\n");
        return NULL;
    }

    QTAILQ_FOREACH(line, &lm->victim_line_list, entry) {
        //printf("Coperd,%s,victim_line_list[%d],ipc=%d,vpc=%d\n", __func__, ++cnt, line->ipc, line->vpc);
        if (line->ipc > max_ipc) {
            victim_line = line;
            max_ipc = line->ipc;
        }
    }

    if (!force && victim_line->ipc < ssd->sp.pgs_per_line / 4) {
        //printf("Coperd,select a victim line: ipc=%d (< 1/8), pgs_per_line=%d\n", victim_line->ipc, ssd->sp.pgs_per_line);
        return NULL;
    }

    QTAILQ_REMOVE(&lm->victim_line_list, victim_line, entry);
    lm->victim_line_cnt--;
    //printf("Coperd,%s,victim_line_list,chooose-victim-block,id=%d,ipc=%d,vpc=%d\n", __func__, victim_line->id, victim_line->ipc, victim_line->vpc);

    /* victim_line is a danggling node now */
    return victim_line;
}

/* here ppa identifies the block we want to clean 
    Returns the number of valid pages we needed to copy within the block*/
static int clean_one_block(struct ssd *ssd, struct ppa *ppa)
{
    struct ssdparams *spp = &ssd->sp;
    struct nand_block *blk = get_blk(ssd, ppa);
    struct nand_page *pg_iter = NULL;
    int cnt = 0;
    int pg;

    for (pg = 0; pg < spp->pgs_per_blk; pg++) {
        ppa->g.pg = pg;
        pg_iter = get_pg(ssd, ppa);
        /* there shouldn't be any free page in victim blocks */
        assert(pg_iter->status != PG_FREE);
        if (pg_iter->status == PG_VALID) {
            gc_read_page(ssd, ppa);
            /* delay the maptbl update until "write" happens */
            gc_write_page(ssd, ppa);
            cnt++;
        }
    }

    assert(blk->vpc == cnt);
    /* do we do "erase" here? */
    return cnt;
}

static void mark_line_free(struct ssd *ssd, struct ppa *ppa)
{
    struct line_mgmt *lm = &ssd->lm;
    struct line *line = get_line(ssd, ppa);
    line->ipc = 0;
    line->vpc = 0;
    /* move this line to free line list */
    QTAILQ_INSERT_TAIL(&lm->free_line_list, line, entry);
    lm->free_line_cnt++;
    //printf("Coperd,%s,one more free line,free_line_cnt=%d\n", __func__, lm->free_line_cnt);
}

static int do_gc(struct ssd *ssd, bool force, NvmeRequest *req)
{
    struct line *victim_line = NULL;
    struct ssdparams *spp = &ssd->sp;
    struct ssd_channel *chp;
    struct nand_lun *lunp;
    struct ppa ppa;
    int ch, lun;
    uint64_t now = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
	uint64_t gc_stime = (ssd->next_ssd_avail_time < now) ? now : \
                     ssd->next_ssd_avail_time;
	//uint64_t gc_stime = ssd->earliest_ssd_lun_avail_time;

    int now_ms = now/1e6;
    int now_s = now/1e9;
    int i;

    if (ssd->sp.enable_gc_sync && !force) {
        //Synchronizing Time Window logic
        if (!ssd->sp.dynamic_gc_sync) {
            int time_window_ms = ssd->sp.gc_sync_window;
            int buffer_ms = ssd->sp.gc_sync_buffer;
            if (ssd->id != (now_ms/time_window_ms) % ssd_id_cnt || 
                // Within buffer of end of GC-window
                ssd->id != ((now_ms+buffer_ms)/time_window_ms) % ssd_id_cnt) {
                return 0;
            }
		// Dynamic Synchronization
        } else {
            //printf("Now (%lld)\n", (long long)now);
			pthread_mutex_lock(&global_gc_endtime_lock);
            if (req->stime < global_gc_endtime + 10000000) { // 10ms buffer
                //printf("Rejecting gc: now (%lld) < global_gc_endtime (%lld)\n", (long long)now, (long long)global_gc_endtime);
				pthread_mutex_unlock(&global_gc_endtime_lock);
                return 0;
            }

			// Is this necessary? This might actually cause more problems! Because this SSD is considered "unavailable"
			/*
			for (ch = 0; ch < spp->nchs; ch++) {
				for (lun = 0; lun < spp->luns_per_ch; lun++) {
					ppa.g.ch = ch;
					ppa.g.lun = lun;
					ppa.g.pl = 0;
					lunp = get_lun(ssd, &ppa);
					lunp->next_lun_avail_time = gc_stime;
				}
			}
			*/
        }
    }

    victim_line = select_victim_line(ssd, force);
    if (!victim_line) {
        //printf("FEMU-FTL: failed to get a victim line!\n");
        //abort();
		if (ssd->sp.enable_gc_sync && !force && ssd->sp.dynamic_gc_sync) {
			pthread_mutex_unlock(&global_gc_endtime_lock);
		}
        return -1;
    }


    if (ssd->sp.enable_free_blocks_log) {
        if (!prev_time_s[ssd->id]) {
            prev_time_s[ssd->id] = now_s;
            ssd->num_gc_in_s = 1;
            ssd->num_valid_pages_copied_s = 0;
        } else if (now_s != prev_time_s[ssd->id]) {
            //printf("Stats,%s,%d,%d,%d\n", ssd->ssdname, prev_time_s[ssd->id], ssd->num_gc_in_s, ssd->num_valid_pages_copied_s);
            prev_time_s[ssd->id] = now_s;
            ssd->num_gc_in_s = 1;
            ssd->num_valid_pages_copied_s = 0;
            for (i=prev_time_s[ssd->id]+1; i < now_s; i++) {
                //printf("Stats,%s,%d,%d,%d\n", ssd->ssdname, i, 0, 0);
            }
        } else {
            ssd->num_gc_in_s++;
        }
    }

    ppa.g.blk = victim_line->id;
    //printf("Coperd,%s,FTL,GCing line:%d,ipc=%d,victim=%d,full=%d,free=%d\n",
            //ssd->ssdname, ppa.g.blk, victim_line->ipc, ssd->lm.victim_line_cnt,
            //ssd->lm.full_line_cnt, ssd->lm.free_line_cnt);
    /* copy back valid data */
    for (ch = 0; ch < spp->nchs; ch++) {
        for (lun = 0; lun < spp->luns_per_ch; lun++) {
            ppa.g.ch = ch;
            ppa.g.lun = lun;
            ppa.g.pl = 0;
            chp = get_ch(ssd, &ppa);
            lunp = get_lun(ssd, &ppa);
            ssd->num_valid_pages_copied_s += clean_one_block(ssd, &ppa);
            //lunp->gc_page_counter += clean_one_block(ssd, &ppa);
            mark_block_free(ssd, &ppa);

            if (spp->enable_gc_delay) {
                struct nand_cmd gce;
                gce.cmd = NAND_ERASE;
                gce.stime = 0;
                ssd_advance_status(ssd, &ppa, &gce);
            }

            chp->gc_endtime = chp->next_ch_avail_time;
            lunp->gc_endtime = lunp->next_lun_avail_time;
            //printf("chp->gc_endtime %lld, global_gc_endtime: %lld\n", (long long)chp->gc_endtime, (long long)global_gc_endtime);
			if (ssd->sp.dynamic_gc_sync) {
				if (lunp->gc_endtime > global_gc_endtime) {
					global_gc_endtime = lunp->gc_endtime;
					//printf("Setting global_gc_endtime: %lld\n", (long long)global_gc_endtime);
				}
			}
			if (lunp->gc_endtime > gc_endtime_array[ssd->id]) {
				gc_endtime_array[ssd->id] = lunp->gc_endtime;
			}
            /*
            lunp->gc_counter++;
            printf("do_gc lunp->gc_counter %d lunp->gc_page_counter %d\n", 
                lunp->gc_counter, lunp->gc_page_counter);
            */
        }
    }

	//printf("SSD%d gc_endtime %"PRIu64" gc_stime %"PRIu64"\n", ssd->id, gc_endtime_array[ssd->id], gc_stime);

	if (ssd->sp.enable_gc_sync && !force && ssd->sp.dynamic_gc_sync) {
		pthread_mutex_unlock(&global_gc_endtime_lock);
	}

    /* update line status */
    mark_line_free(ssd, &ppa);

    return 0;
}

static int do_harmonia_gc(struct ssd *ssd) {
	// Perform GC in all SSDs
	int i;

	pthread_mutex_lock(&harmonia_override_lock);
	for (i=0; i < ssd_id_cnt; i++) {
		harmonia_override[i] = true;
	//	printf("do_harmonia_gc for ssd %d\n", ssd_array[i]->id);
	//	do_gc(ssd_array[i], true);
	}
	pthread_mutex_unlock(&harmonia_override_lock);
	//do_gc(ssd_array[ssd->id], true);
}

static void *ftl_thread(void *arg)
{
    struct ssd *ssd = (struct ssd *)arg;
    NvmeRequest *req = NULL;
    uint64_t lat = 0;
    int rc;
    //unsigned int zero_in_q = 0, one_in_q = 0, two_in_q = 0, three_in_q = 0, four_or_more_in_q = 0;
    //unsigned int total_serviced = 0;

    while (!*(ssd->dataplane_started_ptr)) {
        usleep(100000);
    }

    while (1) {
        if (!ssd->to_ftl || !femu_ring_count(ssd->to_ftl))
            continue;
            /*
        if (femu_ring_count(ssd->to_ftl) == 0) {
            zero_in_q++;
        } else if (femu_ring_count(ssd->to_ftl) == 1) {
            one_in_q++;
        } else if (femu_ring_count(ssd->to_ftl) == 2) {
            two_in_q++;
        } else if (femu_ring_count(ssd->to_ftl) == 3) {
            three_in_q++;
        } else {
            four_or_more_in_q++;
        }
        total_serviced++;
        if (!(total_serviced % 1000)) {
            printf("FEMU: %s serviced %u IOs, 0 in queue: %u, 1 in queue: %u, 2 in queue: %u, 3 in queue: %u, 4+ in queue: %u\n", 
                ssd->ssdname, total_serviced, zero_in_q, one_in_q, two_in_q, three_in_q, four_or_more_in_q);
        }
        */

/*
		pthread_mutex_lock(&harmonia_override_lock);
		if (ssd->sp.harmonia && harmonia_override[ssd->id]) {

			printf("Doing harmonia GC on SSD%d\n", ssd->id);
			do_gc(ssd, true, NULL);

			harmonia_override[ssd->id] = false;
		}
		pthread_mutex_unlock(&harmonia_override_lock);
		*/

        rc = femu_ring_dequeue(ssd->to_ftl, (void *)&req, 1);
        if (rc != 1) {
            printf("FEMU: FTL to_ftl dequeue failed\n");
        }
        assert(req);
        switch (req->is_write) {
            case 1:
                lat = ssd_write(ssd, req);
                break;
            case 0:
                lat = ssd_read(ssd, req);
                if (lat > 1e9) {
                    printf("FEMU: Read latency is > 1s, what's going on!\n");
                }
                break;
            default:
                printf("FEMU: FTL received unkown request type, ERROR\n");
        }

        req->reqlat = lat;
        req->expire_time += lat;
        
        rc = femu_ring_enqueue(ssd->to_poller, (void *)&req, 1);
        if (rc != 1) {
            printf("FEMU: FTL to_poller enqueue failed\n");
        }

		if (req->stime < prev_req_stimes[ssd->id]) {
			//printf("req->stime (%"PRIu64") < prev_req_stime(%"PRIu64"), requests aren't in time order ssd %d!\n", req->stime, prev_req_stimes[ssd->id], ssd->id);
		} else if (req->stime == prev_req_stimes[ssd->id]) {
			//printf("req->stime == prev_req_stime(%"PRIu64"), requests aren't in time order ssd %d!\n", prev_req_stimes[ssd->id], ssd->id);
		}
		prev_req_stimes[ssd->id] = req->stime;

        /* clean one line if needed (in the background) */
        if (should_gc(ssd)) {
			if (ssd->sp.harmonia) {
				printf("FTL: Doing harmonia GC\n");
				do_harmonia_gc(ssd);
			} else {
				do_gc(ssd, false, req);
			}
        }

		pthread_mutex_lock(&harmonia_override_lock);
		if (ssd->sp.harmonia && harmonia_override[ssd->id]) {

			printf("Doing harmonia GC on SSD%d\n", ssd->id);
			do_gc(ssd, true, NULL);

			harmonia_override[ssd->id] = false;
		}
		pthread_mutex_unlock(&harmonia_override_lock);

        // Track and report # of free blocks every 5 seconds
        if (ssd->sp.enable_free_blocks_log) {
            int time_ms = qemu_clock_get_ns(QEMU_CLOCK_REALTIME)/1e6;
            if (!(time_ms % 500) && free_line_print_time[ssd->id] != time_ms) {
                int num_free_blocks = ssd->lm.free_line_cnt * ssd->sp.blks_per_line;
                struct write_pointer *wpp = &ssd->wp;
                struct ssdparams *spp = &ssd->sp;
                // Count # of free blocks in this line
                // only free if not a single page has been written to
                if (!wpp->pg) {
                    num_free_blocks += (spp->tt_luns-(wpp->lun*ssd->sp.nchs+wpp->ch));
                }
                free_line_print_time[ssd->id] = time_ms;
                //printf("Num Free Blocks,%s,%d,%d\n", ssd->ssdname, time_ms,  num_free_blocks);
            }
        }

    }
}

/* accept NVMe cmd as input, in order to support more command types in future */
uint64_t ssd_read(struct ssd *ssd, NvmeRequest *req)
{
    /* TODO: reads need to go through caching layer first */
    /* ... */


    /* on cache miss, read from NAND */
    struct ssdparams *spp = &ssd->sp;
    uint64_t lba = req->slba; /* sector addr */
    int nsecs = req->nlb;
    struct ppa ppa;
    uint64_t start_lpn = lba / spp->secs_per_pg;
    uint64_t end_lpn = (lba + nsecs) / spp->secs_per_pg;
    uint64_t lpn;
    uint64_t sublat, maxlat = 0;
    //struct ssd_channel *ch;
    struct nand_lun *lun;
    bool in_gc = false; /* indicate whether any subIO met GC */
	int i;
	int num_concurrent_gcs = 0;

    if (end_lpn >= spp->tt_pgs) {
        printf("RD-ERRRRRRRRRR,start_lpn=%"PRIu64",end_lpn=%"PRIu64",tt_pgs=%d\n", start_lpn, end_lpn, ssd->sp.tt_pgs);
    }

    //printf("Coperd,%s,end_lpn=%"PRIu64" (%d),len=%d\n", __func__, end_lpn, spp->tt_pgs, nsecs);
    //assert(end_lpn < spp->tt_pgs);
    /* for list of NAND page reads involved in this external request, do: */

    req->gcrt = 0;
#define NVME_CMD_GCT (911)
    if (req->tifa_cmd_flag == NVME_CMD_GCT) {
        /* fastfail IO path */
        for (lpn = start_lpn; lpn <= end_lpn; lpn++) {
            ppa = get_maptbl_ent(ssd, lpn);
            if (!mapped_ppa(&ppa) || !valid_ppa(ssd, &ppa)) {
                //printf("%s,lpn(%" PRId64 ") not mapped to valid ppa\n", ssd->ssdname, lpn);
                //printf("Invalid ppa,ch:%d,lun:%d,blk:%d,pl:%d,pg:%d,sec:%d\n",
                //ppa.g.ch, ppa.g.lun, ppa.g.blk, ppa.g.pl, ppa.g.pg, ppa.g.sec);
                continue;
            }

            //ch = get_ch(ssd, &ppa);
            lun = get_lun(ssd, &ppa);
            if (req->stime < lun->gc_endtime) {
                in_gc = true;
                int tgcrt = lun->gc_endtime - req->stime;
                if (req->gcrt < tgcrt) {
                    req->gcrt = tgcrt;
                }
            } else {
                /* NoGC under fastfail path */
                struct nand_cmd srd;
                srd.cmd = NAND_READ;
                srd.stime = req->stime;
                sublat = ssd_advance_status(ssd, &ppa, &srd);
                maxlat = (sublat > maxlat) ? sublat : maxlat;
            }
        }

		if (in_gc) {
			//printf("TIFA read path: GCRT %d SSD %d\n", req->gcrt, ssd->id);
		}

        if (!in_gc) {
            assert(req->gcrt == 0);
            return maxlat;
        }

        assert(req->gcrt > 0);
        if (maxlat > req->gcrt) {
            //printf("Coperd,%s,%s,%d,inGC,but qlat(%lu) > gclat(%lu)\n", ssd->ssdname, __func__,
                    //__LINE__, maxlat, req->gcrt);
        }

		for (i = 0; i < ssd_id_cnt; i++) {
			if (req->stime < gc_endtime_array[i]) {
				num_concurrent_gcs++;
			}
		}
		if (num_concurrent_gcs > 1) {
			//printf("ssd_read finds possible case of %d concurrent gcs ssd %d\n", num_concurrent_gcs, ssd->id);
		}

        return 0;
    } else {
		int max_gcrt = 0;
        /* normal IO read path */
        for (lpn = start_lpn; lpn <= end_lpn; lpn++) {
            ppa = get_maptbl_ent(ssd, lpn);
            if (!mapped_ppa(&ppa) || !valid_ppa(ssd, &ppa)) {
                //printf("%s,lpn(%" PRId64 ") not mapped to valid ppa\n", ssd->ssdname, lpn);
                //printf("Invalid ppa,ch:%d,lun:%d,blk:%d,pl:%d,pg:%d,sec:%d\n",
                //ppa.g.ch, ppa.g.lun, ppa.g.blk, ppa.g.pl, ppa.g.pg, ppa.g.sec);
                continue;
            }

            lun = get_lun(ssd, &ppa);
            if (req->stime < lun->gc_endtime && max_gcrt < lun->gc_endtime - req->stime) {
				max_gcrt = lun->gc_endtime - req->stime;
            }

            struct nand_cmd srd;
            srd.cmd = NAND_READ;
            srd.stime = req->stime;
            sublat = ssd_advance_status(ssd, &ppa, &srd);
            maxlat = (sublat > maxlat) ? sublat : maxlat;

            /*
            pages_read++;
            if (!(pages_read % 1000))
                printf("pages_read : %u\n", pages_read);

            lun = get_lun(ssd, &ppa);
            if (lun->gc_counter && spp->pg_wr_lat && spp->pg_rd_lat && req->stime < lun->gc_endtime) {
                int pages_left_tocopy = ((int)(lun->gc_endtime-req->stime))/(spp->pg_rd_lat+spp->pg_wr_lat);
                //printf("hello world\n");
                //printf("lun->gc_endtime-req %d\n", ((int)(lun->gc_endtime-req->stime)));
                //printf("spp->pg_rd_lat+spp->pg_wr_lat %d\n", spp->pg_rd_lat+spp->pg_wr_lat);
                maxlat = 20000000;
                //printf("Read blocked by GC, remaining pages to copy: %d\n", pages_left_tocopy);
            } else if (lun->gc_counter) {
                // Reset gc stats
                //printf("Normal Read\n", lun->gc_counter, lun->gc_page_counter);
                if (maxlat > 20000000) {
                    printf("Long Normal Read after GC: %d\n", (int)maxlat);
                    maxlat = 20000000;
                } else {
                    printf("Normal Read after GC: %d\n", (int)maxlat);
                    maxlat = (20000000 > maxlat) ? maxlat : 20000000;
                }
                lun->gc_counter = 0;
                lun->gc_page_counter = 0;
            } else if (maxlat > 20000000) {
                printf("Random Read super long! %d\n", (int)maxlat);
            }
            */
        }

		if (max_gcrt) {
			//printf("Normal read path: GCRT %d SSD %d\n", max_gcrt, ssd->id);
		}
		for (i = 0; i < ssd_id_cnt; i++) {
			if (req->stime < gc_endtime_array[i]) {
				num_concurrent_gcs++;
			}
		}

		ssd->total_reads++;
		if (num_concurrent_gcs) {
			ssd->num_reads_blocked_by_gc[num_concurrent_gcs]++;
		} else {
			ssd->num_reads_blocked_by_gc[0]++;
		}

        if (req->tifa_cmd_flag == 1024 && ssd->sp.enable_gc_sync) {
            printf("tifa_cmd_flag=1024, gc_sync on\n");
            return 100000;
        }

        /* this is the latency taken by this read request */
        //req->expire_time = maxlat;
        //printf("Coperd,%s,rd,lba:%lu,lat:%lu\n", ssd->ssdname, req->slba, maxlat);
        return maxlat;
    }
}

uint64_t ssd_write(struct ssd *ssd, NvmeRequest *req)
{
    uint64_t lba = req->slba;
    struct ssdparams *spp = &ssd->sp;
    int len = req->nlb;
    uint64_t start_lpn = lba / spp->secs_per_pg;
    uint64_t end_lpn = (lba + len - 1) / spp->secs_per_pg;
    struct ppa ppa;
    uint64_t lpn;
    uint64_t curlat = 0, maxlat = 0;
    int r;
    /* TODO: writes need to go to cache first */
    /* ... */

    if (end_lpn >= spp->tt_pgs) {
        printf("ERRRRRRRRRR,start_lpn=%"PRIu64",end_lpn=%"PRIu64",tt_pgs=%d\n", start_lpn, end_lpn, ssd->sp.tt_pgs);
    }
    //assert(end_lpn < spp->tt_pgs);
    //printf("Coperd,%s,end_lpn=%"PRIu64" (%d),len=%d\n", __func__, end_lpn, spp->tt_pgs, len);

    while (should_gc_high(ssd)) {
        /* perform GC here until !should_gc(ssd) */
        //printf("FEMU: FTL doing blocking GC\n");
        r = do_gc(ssd, true, NULL);
        if (r == -1)
            break;
    }

    /* on cache eviction, write to NAND page */

    // are we doing fresh writes ? maptbl[lpn] == FREE, pick a new page
    for (lpn = start_lpn; lpn <= end_lpn; lpn++) {
        ppa = get_maptbl_ent(ssd, lpn);
        if (mapped_ppa(&ppa)) {
            /* overwrite */
            /* update old page information first */
            //printf("Coperd,before-overwrite,line[%d],ipc=%d,vpc=%d\n", ppa.g.blk, get_line(ssd, &ppa)->ipc, get_line(ssd, &ppa)->vpc);
            mark_page_invalid(ssd, &ppa);
            //printf("Coperd,after-overwrite,line[%d],ipc=%d,vpc=%d\n", ppa.g.blk, get_line(ssd, &ppa)->ipc, get_line(ssd, &ppa)->vpc);
            set_rmap_ent(ssd, INVALID_LPN, &ppa);
        }

        /* new write */
        /* find a new page */
        ppa = get_new_page(ssd);
        /* update maptbl */
        set_maptbl_ent(ssd, lpn, &ppa);
        /* update rmap */
        set_rmap_ent(ssd, lpn, &ppa);

        mark_page_valid(ssd, &ppa);

        /* need to advance the write pointer here */
        ssd_advance_write_pointer(ssd);

        struct nand_cmd swr;
        swr.cmd = NAND_WRITE;
        swr.stime = req->stime;
        /* get latency statistics */
        curlat = ssd_advance_status(ssd, &ppa, &swr);
        maxlat = (curlat > maxlat) ? curlat : maxlat;
    }

    return maxlat;
}

