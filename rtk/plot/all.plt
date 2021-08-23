set terminal postscript eps enhanced color font ",44"
set output "eps/all.eps"
set grid lt 1 lc 'gray' lw 1

set boxwidth 1
set style fill pattern border
set tmargin 1
set size 3.8,.80
#set key center center samplen 4

set multiplot layout 1,9 title "{/:Bold Read Latency CDFs}" offset 40,-1.8

#### Overall legend for all the CDFs (sub-plots) below

#set origin .0,.4
#set size 3.35,0.35
#unset key
#set key horizontal box at 8,1200.25 #width -1
#set border 0
#unset tics
#unset xlabel
#unset ylabel
#set title "Read Latency CDFs" offset 0,2
#set yrange [0:1]
#plot \
#    2 t 'Base'  lc rgb "red" lw 30, \
#    2 t 'IOD_1' lc rgb "brown" dt 4 lw 30, \
#    2 t 'IOD_2'   lc rgb "orange" dt 4 lw 30, \
#    2 t 'IOD_3'   lc rgb "blue" dt 4 lw 30, \
#    2 t 'IODA'  lc rgb "green" lw 30, \
#    2 t 'Ideal'  lc rgb "gray" lw 30

# overall xtic
set origin 0.00,0.0
set size 3.2,0.15
set yrange [0:1]
unset border
set border 0
unset tics
unset xlabel
unset ylabel
unset key
set key horizontal box samplen 1.5 at 1.0,-0.0 width -1.5
set label "{Latency (ms)}" at 1.5,-1
plot \
    2 t 'Base'  lc rgb "red" lw 30, \
    2 t 'IOD_1' lc rgb "brown" dt 4 lw 30, \
    2 t 'IOD_2'   lc rgb "orange" dt 4 lw 30, \
    2 t 'IOD_3'   lc rgb "blue" dt 4 lw 30, \
    2 t 'IODA'  lc rgb "green" lw 30, \
    2 t 'Ideal'  lc rgb "gray" lw 30

##############################################################
set border
# 1st plot

set origin 0,0
set size 0.63,0.8
set title ""
set xrange [0:40]
set yrange [0.96:1]
#set xlabel "Latency (ms)"
#set ytics ('.95' .95, '.96' .96, '.97' .97, '.98' .98, '.99' .99, '1' 1)
set ytics out nomirror ('.96' .96, '.97' .97, '.98' .98, '.99' .99, '1' 1) offset .5,0
unset key
#set key top center outside horizontal
set xtics out nomirror 20 offset 0,.5 font ",36"
unset label
set label center "[a] Azure" at 19,.97 front

plot \
'dat/azurestorage/azurestorage-base.dat' u ($1/1000):2 t "Base" w l lc rgb "red" lw 20, \
'dat/azurestorage/azurestorage-iod1.dat' u ($1/1000):2 t "iod1" w l lc rgb "brown" dt 3 lw 12, \
'dat/azurestorage/azurestorage-iod2.dat' u ($1/1000):2 t "iod2" w l lc rgb "orange" dt 4 lw 11, \
'dat/azurestorage/azurestorage-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 6 lw 17, \
'dat/azurestorage/azurestorage-ioda.dat' u ($1/1000):2 t "TIFA" w l lc rgb "green" lw 17, \
'dat/azurestorage/azurestorage-ideal.dat' u ($1/1000):2 t "ideal" w l lc rgb "gray" lw 11, \

# 2nd plot

set origin 0.48,0
set size 0.55,0.8
set title ""
set xrange [0:40]
set yrange [0.96:1]
#set xlabel "Latency (ms)"
#set ytics ('.98' .98, '.99' .99, '1' 1)
#set ytics ('' .95, '' .96, '' .97, '' .98, '' .99, '' 1)
set ytics out nomirror ('' .96, '' .98, '' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
set xtics out nomirror 20 offset 0,.5
unset label
unset key
set label center "[b] BingIdx" at 22,.97 front
set label left "IODA close to Ideal" at -12,1.004 font ",38"
#set label left "to Ideal" at -4,.9938 font ",18"
set arrow from 8.8,1.0015 to 12.3,.992 head filled front lw 9 lc rgb "black"

#set grid

plot \
'dat/bingindex/bingindex-base.dat' u ($1/1000):2 t "Base" w l lc rgb "red" lw 20, \
'dat/bingindex/bingindex-iod1.dat' u ($1/1000):2 t "EBUSY" w l lc rgb "brown" dt 3 lw 12, \
'dat/bingindex/bingindex-iod2.dat' u ($1/1000):2 t "GCT" w l lc rgb "orange" dt 4 lw 11, \
'dat/bingindex/bingindex-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/bingindex/bingindex-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/bingindex/bingindex-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \


# 3rd plot

set origin .88,0
set size 0.55,0.8
set title ""
set xrange [0:40]
set yrange [0.96:1]
#set xlabel "Latency (ms)"
#set ytics ('.98' .98, '.99' .99, '1' 1)
#set ytics ('' .95, '' .96, '' .97, '' .98, '' .99, '' 1)
set ytics out nomirror ('' .96, '' .98, '' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
set xtics out nomirror 20
#set grid
unset label
unset arrow
set label center "[c] BingSel" at 20,.97 front

plot \
'dat/bingselect/bingselect-base.dat' u ($1/1000):2 t "Baseline" w l lc rgb "red" lw 20, \
'dat/bingselect/bingselect-iod1.dat' u ($1/1000):2 t "iod1" w l lc rgb "brown" dt 3 lw 12, \
'dat/bingselect/bingselect-iod2.dat' u ($1/1000):2 t "iod2" w l lc rgb "orange" dt 4 lw 11, \
'dat/bingselect/bingselect-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/bingselect/bingselect-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/bingselect/bingselect-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \

# 4th plot

set origin 1.28,0
set size 0.55,0.8
set title ""
set xrange [0:40]
set yrange [0.96:1]
#set xlabel "Latency (ms)"
set tics in
#set ytics ('.95' .95, '.96' .96, '.97' .97, '.98' .98, '.99' .99, '1' 1)
unset ytics
#set ytics out nomirror ('.96' .96, '.98' .98, '1' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
set xtics out nomirror 20
unset label
set label center "[d] Cosmos" at 20,.97 front

plot \
'dat/cosmos/cosmos-base.dat' u ($1/1000):2 t "Baseline" w l lc rgb "red" lw 20, \
'dat/cosmos/cosmos-iod1.dat' u ($1/1000):2 t "iod1" w l lc rgb "brown" dt 3 lw 12, \
'dat/cosmos/cosmos-iod2.dat' u ($1/1000):2 t "iod2" w l lc rgb "orange" dt 4 lw 11, \
'dat/cosmos/cosmos-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/cosmos/cosmos-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/cosmos/cosmos-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \


# 5th plot:


set origin 1.68,0
set size 0.55,0.8
set title ""
set xrange [0:10]
set yrange [0.96:1]
#set xlabel "Latency (ms)"
set tics in
set xtics out nomirror 5
#set ytics ('.98' .98, '.99' .99, '1' 1)
#set ytics ('' .95, '' .96, '' .97, '' .98, '' .99, '' 1)
set ytics out nomirror ('' .96, '' .98, '' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
#set grid
unset label
set label center "[e] DTRS" at 5.5,.97 front

plot \
'dat/dtrs/dtrs-base.dat' u ($1/1000):2 t "Baseline" w l lc rgb "red" lw 20, \
'dat/dtrs/dtrs-iod1.dat' u ($1/1000):2 t "iod1" w l lc rgb "brown" dt 3 lw 12, \
'dat/dtrs/dtrs-iod2.dat' u ($1/1000):2 t "iod2" w l lc rgb "orange" dt 4 lw 11, \
'dat/dtrs/dtrs-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/dtrs/dtrs-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/dtrs/dtrs-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \


# 6th plot

set origin 2.08,0
set size 0.55,0.8
set title ""
set xrange [0:40]
set yrange [0.96:1]
#set xlabel "Latency (ms)" # in logscale

#set xtics out nomirror ('.1' 100, '.2' 200, '.5' 500, '1' 1000, '2' 2000, '4' 4000)
#set ytics ('.98' .98, '.99' .99, '1' 1)
#set ytics ('' .95, '' .96, '' .97, '' .98, '' .99, '' 1)
set ytics out nomirror ('' .96, '' .98, '' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
set xtics out nomirror 20
unset label
set label center "[f] Exch" at 25,.97 front



plot \
'dat/est/est-base.dat' u ($1/1000):2 t "Baseline" w l lc rgb "red" lw 20, \
'dat/est/est-iod1.dat' u ($1/1000):2 t "EBUSY" w l lc rgb "brown" dt 3 lw 12, \
'dat/est/est-iod2.dat' u ($1/1000):2 t "GCT" w l lc rgb "orange" dt 4 lw 11, \
'dat/est/est-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/est/est-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/est/est-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \

#7th plot

set origin 2.48,0
set size 0.55,0.8
set title ""
set xrange [0:40]
set yrange [0.96:1]
#set ytics ('.95' .95, '.96' .96, '.97' .97, '.98' .98, '.99' .99, '1' 1)
unset ytics
#set ytics out nomirror ('.96' .96, '.98' .98, '1' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
set xtics out nomirror 20
unset label
set label center "[g] LMBE" at 16,.97 front
#show label

plot \
'dat/lmbe/lmbe-base.dat' u ($1/1000):2 t "Baseline" w l lc rgb "red" lw 20, \
'dat/lmbe/lmbe-iod1.dat' u ($1/1000):2 t "iod1" w l lc rgb "brown" dt 3 lw 12, \
'dat/lmbe/lmbe-iod2.dat' u ($1/1000):2 t "iod2" w l lc rgb "orange" dt 4 lw 11, \
'dat/lmbe/lmbe-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/lmbe/lmbe-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/lmbe/lmbe-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \

#8th plot

set origin 2.88,0
set size 0.55,0.8
set title ""
set xrange [0:30]
set yrange [0.96:1]
#set ytics ('.98' .98, '.99' .99, '1' 1)
#set ytics ('' .95, '' .96, '' .97, '' .98, '' .99, '' 1)
set ytics out nomirror ('' .96, '' .98, '' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
set xtics out nomirror 15
unset label
set label center "[h] MSNFS" at 16,.97 front

plot \
'dat/msnfs/msnfs-base.dat' u ($1/1000):2 t "Baseline" w l lc rgb "red" lw 20, \
'dat/msnfs/msnfs-iod1.dat' u ($1/1000):2 t "iod1" w l lc rgb "brown" dt 3 lw 12, \
'dat/msnfs/msnfs-iod2.dat' u ($1/1000):2 t "iod2" w l lc rgb "orange" dt 4 lw 11, \
'dat/msnfs/msnfs-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/msnfs/msnfs-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/msnfs/msnfs-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \

#9th plot

set origin 3.28,0
set size 0.55,0.8
set title ""
set xrange [0:40]
set yrange [0.96:1]
#set ytics ('.98' .98, '.99' .99, '1' 1)
#set ytics ('' .95, '' .96, '' .97, '' .98, '' .99, '' 1)
set ytics out nomirror ('' .96, '' .98, '' 1) offset .5,0
set ytics out nomirror ('' .96, '' .97, '' .98, '' .99, '' 1) offset .5,0
set xtics out nomirror 20
unset label
set label center "[i] TPCC" at 21,.97 front

plot \
'dat/tpcc/tpcc-base.dat' u ($1/1000):2 t "Baseline" w l lc rgb "red" lw 20, \
'dat/tpcc/tpcc-iod1.dat' u ($1/1000):2 t "EBUSY" w l lc rgb "brown" dt 3 lw 12, \
'dat/tpcc/tpcc-iod2.dat' u ($1/1000):2 t "GCT" w l lc rgb "orange" dt 4 lw 11, \
'dat/tpcc/tpcc-iod3.dat' u ($1/1000):2 t "TW-Only" w l lc rgb "blue" dt 4 lw 17, \
'dat/tpcc/tpcc-ioda.dat' u ($1/1000):2 t "iod2+TW" w l lc rgb "green" lw 17, \
'dat/tpcc/tpcc-ideal.dat' u ($1/1000):2 t "NoGC" w l lc rgb "gray" lw 11, \

