# Segments

Originally a bash script, I replaced it with a "C" binary. While not as portable as a script, the reality is, the complexity for some of the tasks of the original script, made the script itself, unwieldy.

Purpose:

I tend to reuse text segments in code files, particularly scripts. I also like to replace or update text blocks in files.

This program is designed to extract, replace or list delimited segments of text in files, scripts, c-code files, text files, xml, anything text.

The format of the delimiters have to be a single line and unique to all the rest of the text in the file. The segments can be named, the name follows the segment delimiter. If a name is used, both the opening and closing delimiters must have the name. This allows for embedded segments.

The delimiters are free form, but there are three defaults, "code-begin, code-end", "begin-segment, end-segment" and "begin-marker, end-marker".

You can add customer delimiters using the "-b" and "-e" options at runtime. The delimiter pairs can be different or the same. I most cases it's easier to read them in code if they are different. In code files, the delimiters *should* be inside single line comment fields specific to the code language.

You can also select the delimiter pair using the "-d" option and see all the current delimiters using the "-s" option. If you add a custom delimiter pair, it becomes the default operational pair during the run.

If you *do not* select a delimiter pair before extracting, replacing or listing, the program will attempt to autodetect the delimiters from the current set of delimiters in memory.

You can operating on segments simply by the delimiter or by the delimiter segment-name pair. As an example, you name a segment thusly...

\# [ code-begin SegmentName ]

In this bash script comment, the marker is "code-begin" and named "SegmentName". There must be white space between the marker and the name and the name and trailing end, unless it is a newline (newlines are considered white space). In this example, I decorated the marker with "[" and "]", these are superfleus, but they do help with readability.
