set term postscript noenhanced eps color 20
set title "CDF of Read (tpcc-resize-w16.0-20s)"
set key right bottom
set xrange [0:70]
set yrange[0.8:1]
set xlabel "Latency (ms)"

set output "eps/tpcc-resize-w16.0-20s.eps"
plot \
"dat/sosp21-tpcc-resize-w16.0-20s/sosp21-nogc-tpcc-resize-w16.0-20s-nosync-nopgc-1-rd_lat.dat" u ($1/1000):2 t "ideal" w l lc rgb "grey" lw 5, \
"dat/sosp21-tpcc-resize-w16.0-20s/sosp21-ktw-tpcc-resize-w16.0-20s-sync100ms-nopgc-2-rd_lat.dat" u ($1/1000):2 t "ioda" w l lc rgb "green" lw 5, \
"dat/sosp21-tpcc-resize-w16.0-20s/sosp21-gct-tpcc-resize-w16.0-20s-sync100ms-nopgc-3-rd_lat.dat" u ($1/1000):2 t "iod3" w l lc rgb "blue" lw 5, \
"dat/sosp21-tpcc-resize-w16.0-20s/sosp21-gct-tpcc-resize-w16.0-20s-nosync-nopgc-3-rd_lat.dat" u ($1/1000):2 t "iod2" w l lc rgb "orange" lw 5, \
"dat/sosp21-tpcc-resize-w16.0-20s/sosp21-ebusy-tpcc-resize-w16.0-20s-nosync-nopgc-3-rd_lat.dat" u ($1/1000):2 t "iod1" w l lc rgb "brown" lw 5, \
"dat/sosp21-tpcc-resize-w16.0-20s/sosp21-def-tpcc-resize-w16.0-20s-nosync-nopgc-1-rd_lat.dat" u ($1/1000):2 t "base" w l lc rgb "red" lw 5, \
