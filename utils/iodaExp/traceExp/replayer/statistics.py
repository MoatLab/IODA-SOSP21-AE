#!/usr/bin/env python
#title           :statistics.py
#author          :Vincentius Martin
#==============================================================================

from operator import itemgetter

if __name__ == '__main__':
    sorted_io = []

    readbandwidth = 0
    readlatency = 0
    totalread = 0
    writebandwidth = 0
    writelatency = 0
    totalwrite = 0

    last_io_time = -1
    last_write_time = -1
    inter_io_time = 0
    inter_write_time = 0

    with open("replay_metrics.txt") as f:
        for line in f:
            tok = map(str.strip, line.split(","))
            sorted_io.append([float(tok[0]),int(tok[1]),float(tok[2]),int(tok[3]),float(tok[4])])


    for io in sorted(sorted_io, key=itemgetter(0)):
        if (io[2] == 1): #read
            readbandwidth += (io[3]/1024) / (io[1]/1000000.0)
            readlatency += io[1]
            totalread += 1
        else: #write
            writebandwidth += (io[3]/1024) / (io[1]/1000000.0)
            writelatency += io[1]
            totalwrite += 1
            if last_write_time != -1:
                inter_write_time += io[0] - last_write_time
            last_write_time = io[0]

        if last_io_time != -1:
            inter_io_time += io[0] - last_io_time
        last_io_time = io[0]

    print "==========Statistics=========="
    print "Last time " + str(last_io_time)
    print "IO inter arrival time average " + "%.2f" % (inter_io_time / (totalread + totalwrite - 1)) + " ms"
    print "Write inter arrival time average " + "%.2f" % (inter_write_time / (totalwrite - 1))
    print "Total writes: " + str(totalwrite)
    print "Total reads: " + str(totalread)
    print "Write iops: " + "%.2f" % (float(totalwrite) / (last_io_time / 1000))
    print "Read iops: " + "%.2f" % (float(totalread) / (last_io_time / 1000))
    print "Average write bandwidth: " + "%.2f" % (writebandwidth / totalwrite) + " KB/s"
    print "Average write latency: " + "%.2f" % (writelatency / totalwrite) + " us"
    print "Average read bandwidth: " + "%.2f" % (readbandwidth / totalread) + " KB/s"
    print "Average read latency: " + "%.2f" % (readlatency / totalread) + " us"
    print "=============================="

