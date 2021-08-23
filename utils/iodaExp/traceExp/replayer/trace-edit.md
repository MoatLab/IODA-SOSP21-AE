<h1> Trace Editor </h1>

<p>
To run, access the trace-editor py in the root directory. <br />
Please use the correct input for now, I haven't put any advanced validation. <br />

Before running, create 2 symlinks/folders inside this directory: <br />
./in: contains all input files <br />
./out: contains all output files <br />
<br />
The scripts will take every input and produce every output to those directories. <br />
<br />
Please keep in mind that every trace must be preprocessed first before getting into script's another functionalities.
</p>

<h2>List of commands: </h2>
<p>
1. Preprocess a trace or traces inside a directory.<br />
Type of traces:<br/>
<li>
  <ol>Microsoft Server Trace</ol>
  <ol>BlkReplay's blktrace</ol>
  <ol>Unix's blktrace: in our case, so far it is the same with Hadoop trace</ol>
</li>
</p>
<pre>python trace-editor.py -file &lt;tracename&gt; -preprocessMSTrace (-filter read/write)</pre>
<pre>python trace-editor.py -file &lt;tracename&gt; -preprocessBlkReplayTrace (-filter read/write)</pre>
<pre>python trace-editor.py -file &lt;tracename&gt; -preprocessUnixBlkTrace (-filter read/write)</pre>

<p>It can also preprocess all traces inside a directory, here's an example using MS-Trace</p>
<pre>python trace-editor.py -dir &lt;dirname&gt; -preprocessMSTrace (-filter read/write)</pre>

<p>
2. Modify a trace (Precondition: The trace must has been preprocessed)<br />
Resize all requests size by 2x and rerate all request arrival time by 0.5x : <br />
</p>
<pre>python trace-editor.py -file &lt;tracename&gt; -resize 2 -rerate 0.5</pre>
<p>
Insert a 4KB read (iotype == 1) for every 1000ms: <br />
</p>
<pre>python trace-editor.py -file &lt;tracename&gt; -insert -size 4 -iotype 1 -interval 1000</pre>

<p>
3. Combine traces (Precondition: The traces must have been preprocessed).<br />
Make sure that the traces' names are well ordered because the script will just do the process without ordering the traces.
Well ordered means the traces are ordered from the earliest time to the latest time. Just check this condition with -ls.
</p>
<pre>python trace-editor.py -dir &lt;dirname&gt; -combine</pre>

<p>
4. Merge traces (Precondition: The traces must have been preprocessed).<br />
Merge the traces in a directory, all timestamps will be subtracted with the lowest timestamp.
</p>
<pre>python trace-editor.py -dir &lt;dirname&gt; -merge</pre>

<p>
5. Break to RAID-0 disks
In this example get RAID disks from 4 disks with the stripe unit size 65536 bytes
</p>

<pre>python trace-editor.py -breaktoraid -file &lt;infile&gt; -ndisk 4 -stripe 65536</pre>

<p>
6. Check IO imbalance in the RAID Disks.
This example uses 3disks with the granularity of 5minutes.
</p>

<pre>python trace-editor.py -ioimbalance -file &lt;filename&gt; -granularity 5</pre>

<p>
7. Check the busiest or the most loaded (in kB) time for a specific disk in a directory <br />
Busiest = a time range with the largest number of requests <br />
Most Loaded = a time range with the largest total requests size <br />
<br />
Notes: <br />
duration - in hrs, in this example 1hrs (60mins) <br />
top - top n result in this example 3 top results <br />
</p>
<pre>python trace-editor.py -dir &lt;dirname&gt; -mostLoaded -duration 60 -top 3</pre>
<pre>python trace-editor.py -dir &lt;dirname&gt; -busiest -duration 60 -top 3</pre>

<p> Check the largest average time, the usage is the same with busiest and most loaded </p>
<pre>python trace-editor.py -dir &lt;dirname&gt; -busiest -duration 60 -top 3</pre>

<p>
8. Top Large IO, In this example: <br />
Top 3 Large IO with size greater than or equal 64kB, with 1hr duration
</p>

<pre>python trace-editor.py -toplargeio -file &lt;filename&gt; -offset 64 -devno 0 -duration 60 -top 3</pre>

<p>
9. Find most random write time range, In this example: <br />
Find a time range(min) where has most random write
</p>

<pre>python trace-editor.py -dir &lt;dirname&gt; -mostRandomWrite -duration 5 -devno 5 -top 3</pre>

<p>
10. Get characteristic info from a after-preprocessed trace(usually after you cut the original preprocessed trace, due to devno reason), In this example: <br />
You can get something like whisker plot info about write size, read size, time density, and % write, % read, % random write
</p>

<pre>python trace-editor.py -dir &lt;dirname&gt; -characteristic</pre>

<p>
11. Cut trace, in this example between timerange of minute 5 and minute 10
</p>

<pre>python trace-editor.py -cuttrace -file &lt;filename&gt; -timerange 5 10</pre>

<p>
12. Sanitize the trace (incorporate contiguous IO + remove repeated reads)
</p>

<pre>python trace-editor.py -file &lt;filename&gt; -sanitize</pre>

<hr />

<h1> Simple Replayer </h1>

<p> 
Use this to replay a trace (must be in disksim format) <br />
To compile: gcc replay.c -pthread <br /><br />
With the default C compilation, let's say it results in a.out <br />
Then run it with: ./a.out &lt;target_device&gt; <br /><br />
To configure, open the source code and edit the global variables in CONFIGURATION PART
