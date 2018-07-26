set terminal postscript eps enhanced color solid lw 1 "DejaVuSans" 12
set output "out.eps"
set style data linespoint
set multiplot layout 2,1
plot "out.dat" using 1:2 lt 1 pi -1 pt 7 ps 0.5 title 'Average Loss', "out.dat" using 1:3 lt 3 pi -1 pt 13 ps 0.5 title 'RTT'
plot "out.dat" using 1:4 lt 1 pi -1 pt 7 ps 0.5 title 'Bitrate',"out.dat" using 1:6 lt 3 pi -1 pt 13 ps 0.5 title 'Threshold'
unset multiplot
