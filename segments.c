#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <regex.h>
#include <libgen.h>
#include <stdarg.h>

#define BUFFER_SIZE 2048
// #define DEBUGMODE 1

// Segment Pair Struct
typedef struct segment_pair_struct
{
	char *beginMarker;
	char *endMarker;
	char *segmentName;
} segment_pair;

// Segment Struct
typedef struct segment_struct
{
	char *segment;
	char *name;
	char *source;
	int startLine, endLine;
} segment;

// OpMode Enumeration
typedef enum { extract, replace, extractall, list, test } OpModes;

// Default Segment Pairs + one user definable slot
segment_pair **pairs = NULL;
// OpMode
OpModes opMode = extract;
// Source Filename
char *source = NULL;
// Remove Source Flag
int removeSource = 0;		// In the event data was from stdin, it must be saved for processing, but should be deleted afterwards
// Output Filename
char *output = NULL;
// Replacement Segment
char *replacement = NULL;
// Replacement Count
int replacementCount = 1;
// Segment Name
char *segName = NULL;
// Individual Flag
int individual = 0;
// Current Pair
int currentPair  = -1;
// Total Pairs
int totalPairs = 0;

// Trace Output
FILE *traceptr = NULL;

// Usage : Show Help Menu
void Usage()
{
	fprintf(stderr,"==========================================================\n");
	fprintf(stderr,"segement : Replace or Extract Delimited Segments of a file\n");
	fprintf(stderr,"==========================================================\n");
	fprintf(stderr,"-h\t\tThis Menu\n");
	fprintf(stderr,"-x\t\tExtract segment mode\n");
	fprintf(stderr,"-r\t\tReplace segment mode\n");
	fprintf(stderr,"-a\t\tExtract all segments mode\n");
	fprintf(stderr,"-1\t\tMake sure segments go to files, not to stdout\n");
	fprintf(stderr,"-s\t\tShow default segment delimiter sets\n");
	fprintf(stderr,"-n [name]\tName of specific segment\n");
	fprintf(stderr,"-f [file]\tInput file\n");
	fprintf(stderr,"-o [file]\tOutput file\n");
	fprintf(stderr,"-i [file]\tReplacement segment\n");
	fprintf(stderr,"-c [count]\tReplacement count\n");
	fprintf(stderr,"-d [num]\tSelect segment delimiter from default set\n");
	fprintf(stderr,"-b [regex]\tSet custom segment delimiter start marker\n");
	fprintf(stderr,"-e [regex]\tSet custom segment delimiter end marker\n");
}

// Close trace file
void TraceClose()
{
	#ifdef DEBUGMODE

	if (traceptr != stderr && traceptr != stdout && traceptr != NULL)
		fclose(traceptr);

	#endif
}

// Create New Trace File
void NewTrace(char *tracefile)
{
	#ifdef DEBUGMODE

	TraceClose();

	if (tracefile == NULL || !strcmp(tracefile,""))
		traceptr = stderr;
	else
		traceptr = fopen(tracefile,"w");

	#endif
}

// Trace : Debug Trace File
void Trace(const char *fmt,...)
{
	#ifdef DEBUGMODE

	if (traceptr != NULL)
	{
		char buffer[BUFFER_SIZE];
		va_list vl;
		va_start(vl,fmt);

		memset(buffer,0,BUFFER_SIZE);

		vsprintf(buffer,fmt,vl);

		fprintf(traceptr,"%s",buffer);
	}

	#endif
}

// ShowSegmentDelimiters : Show Default Delimiters
void ShowSegmentDelimiters()
{
	int maxItems = totalPairs;

	maxItems = (strcmp(pairs[maxItems-1]->beginMarker,"") == 0) ? maxItems - 1 : maxItems;

	fprintf(stderr,"Index\tBegin Marker / End Marker\n");
	fprintf(stderr,"=================================\n");

	for (int index=0; index < maxItems; ++index)
		fprintf(stderr,"%d :\t%s / %s\n",index,pairs[index]->beginMarker,pairs[index]->endMarker);
}

// charset : Determine if given char is in set of chars
int charset(const char c, const char *set)
{
	int charclass = strlen(set);
	int found = 0;

	for (int index=0; index < charclass && !found; ++index)
	{
		found = (c == set[index]);
	}

	return (found);
}

// strblock : Find end of block, chars are the delimiters
int strblock(const char *inputstr, const char *chars)
{
	int length = strlen(inputstr);
	int found = -1;

	for (int index=0; index < length && found < 0; ++index)
	{
		if (charset(inputstr[index],chars))
		{
			found = index;
		}
	}

	return (found);
}

// strnot : Find first character not from chars
char *strnot(const char *inputstr, const char *chars)
{
	char *ptr = (char *) inputstr;
	int length = strlen(inputstr);
	int found = 0;

	for (int index=0; index < length && !found; ++index)
	{
		if (!charset(inputstr[index],chars))
		{
			found = !found;
			ptr += index;
		}
	}

	return (ptr);
}

// strchars : Find first of supplied chars
char *strchars(const char *inputstr, const char *chars)
{
	char *ptr = (char *) inputstr;
	int length = strlen(inputstr);
	int found = 0;

	for (int index=0; index < length && !found; ++index)
	{
		if (charset(inputstr[index],chars))
		{
			ptr += index;

			found = !found;
		}
	}

	return (ptr);
}

// strnext : Find next string
char *strnext(const char *inputstr)
{
	static char buffer[1024];
	char *whitespace = strchars(inputstr," \t");

	memset(buffer,0,sizeof(buffer));

	if (whitespace != NULL)
	{
		char *nextblk = strnot(whitespace," \t");

		if (nextblk != NULL)
		{
			int blklen = strblock(nextblk," \t\n\r");
			char block[1024];

			memset(block,0,sizeof(block));

			if (blklen > -1 && blklen < 1024)
			{
				strncpy(block,nextblk,blklen);

				if (strlen(block) > 0)
					strcpy(buffer,block);
			}
		}
	}

	return buffer;
}

// Create Dynamic String
char *DynString(char *ptr)
{
	char *output = NULL;

	if (ptr != NULL)
	{
		int len = strlen(ptr);

		if (len > 0)
		{
			output = calloc(sizeof(char),len);

			strcpy(output,ptr);
		}
	}

	return (output);
}

// Add Segment Delimit Pair
void AddPair(char *begin, char *end, char *segname)
{
	if (begin != NULL)
	{
		segment_pair **tmp = (segment_pair **) calloc(sizeof(segment_pair*), totalPairs + 1);
		memcpy(tmp,pairs,totalPairs * sizeof(segment_pair*));

		tmp[totalPairs] = (segment_pair*) malloc(sizeof(segment_pair));
		tmp[totalPairs]->beginMarker = DynString(begin);
		tmp[totalPairs]->endMarker = (end == NULL) ? tmp[totalPairs]->beginMarker : DynString(end);
		tmp[totalPairs]->segmentName = (segname == NULL) ? NULL : DynString(segname);

		free(pairs);

		pairs = tmp;

		++totalPairs;
	}
}

// Allocate begin-end Pairs
void Allocate()
{
	AddPair("begin-segment","end-segment",NULL);
	AddPair("code-begin","code-end",NULL);
	AddPair("begin-marker","end-marker",NULL);
}

// Deallocate begin-end pairs
void Deallocate()
{
	for (int index=0; index < totalPairs; ++index)
	{
		if (pairs[index]->endMarker != NULL && pairs[index]->endMarker != pairs[index]->beginMarker)
			free(pairs[index]->endMarker);

		if (pairs[index]->segmentName != NULL)
			free(pairs[index]->segmentName);

		free(pairs[index]->beginMarker);

		free(pairs[index]);
	}

	free(pairs);

	pairs = NULL;
}

// Process Cmdline Args
void ProcessArgs(int argc, char **argv)
{
	char *optString = "b:e:f:i:d:n:o:1srlxath?";
	char opt = getopt(argc,argv,optString);

	while (opt != -1)
	{
		switch (opt)
		{
		case 'h':		// Help
		case '?':
			Usage();
			exit(0);
			break;
		case 'n':		// Segment Name
			segName=optarg;
			break;
		case 'f':		// Source Filename
			source=optarg;
			break;
		case 'o':		// Output Filename
			output=optarg;
			break;
		case 'i':		// Replacement segment
			replacement=optarg;
			break;
		case 'c':		// Replacement Count
			replacementCount=atoi(optarg);
			replacementCount = (replacementCount < 1) ? 1 : replacementCount;
			break;
		case 'r':		// Replace Mode
			opMode = replace;
			break;
		case 'l':		// List mode
			opMode = list;
			break;
		case 'x':		// Extract Mode
			opMode = extract;
			break;
		case 'a':		// Extract All Mode
			opMode = extractall;
			break;
		case 't':		// Test Mode
			opMode = test;
			break;
		case 's':		// Show Segement Delimiters
			ShowSegmentDelimiters();
			exit (0);
			break;
		case '1':		// When Extracting All, save all segments to individual files
			individual = 1;
			break;
		case 'd':		// Select Delimiter
			currentPair=atoi(optarg);
			currentPair=(currentPair >= totalPairs) ? 0 : currentPair;
			break;
		case 'b':		// User Define Segment Begin Delimiter
			AddPair(optarg,NULL,NULL);
			break;
		case 'e':		// USer Defined Segement End Delimiter
			pairs[totalPairs-1]->endMarker=DynString(optarg);
			break;
		}

		opt = getopt(argc,argv,optString);
	}
}

// Determine If Begin Marker Appears In String
char *BeginMark(const char *str)
{
	char *ptr = strstr(str,pairs[currentPair]->beginMarker);

	return (ptr);
}

// Determine if End Marker Appears In String
char *EndMark(const char *str)
{
	char *ptr = strstr(str,pairs[currentPair]->endMarker);

	return (ptr);
}

// InsertFile : Insert a data from file into currently open file
int InsertFile(FILE *ptr,char *datafile)
{
	int retValue = 0;

	FILE *inptr = fopen(datafile,"r");

	if (inptr != NULL)
	{
		char buffer[BUFFER_SIZE];

		fgets(buffer,BUFFER_SIZE-1,inptr);

		do {
			fprintf(ptr,"%s",buffer);
		} while (fgets(buffer,BUFFER_SIZE-1,inptr) != NULL);

		fclose(inptr);
	}
	else
		retValue = 127;

	return retValue;
}

// Find Segment Delimiter In File (If not supplied)
int FindSegmentDelimiter()
{
	FILE *ptr = NULL;
	FILE *outptr = NULL;
	int found = -1;

	if (source == NULL)
	{
		static char tmpname[1024];

		memset(tmpname,0,sizeof(tmpname));
		strcpy(tmpname,"/tmp/segments.XXXXXX");

		int fd = mkstemp(tmpname);
		close(fd);

		source = tmpname;

		outptr = fopen(source,"w");
		ptr = stdin;

		removeSource = 1;
	}
	else
	{
		ptr = fopen(source,"r");
	}

	if (ptr != NULL)
	{
		char buffer[BUFFER_SIZE];

		memset(buffer,0,sizeof(buffer));

		fgets(buffer,BUFFER_SIZE-1,ptr);

		do {
			if (outptr != NULL)
				fprintf(outptr,"%s",buffer);

			for (int index=0; index < totalPairs; ++index)
			{
				if (strstr(buffer,pairs[index]->beginMarker) != NULL)
				{
					found = index;

					break;
				}
			}
		} while (fgets(buffer,BUFFER_SIZE-1,ptr) != NULL && found == -1);

		if (ptr != stdin)
			fclose(ptr);

		if (outptr != NULL)
			fclose(outptr);
	}
	else
	{
		fprintf(stderr,"Could not open %s\n",source);
	}

	return (found);
}

// Extract Function
int Extract()
{
	int retValue = 0;
	segment_pair *pair = pairs[currentPair];
	char *delimiters[] = { pair->beginMarker, pair->endMarker };
	int dindex = 0, found = 0, insegment = 0, segcount = 0;

	if (segName != NULL)
	{
		pair->segmentName=DynString(segName);
	}

	char buffer[BUFFER_SIZE];
	FILE *ptr = (source == NULL) ? stdin : fopen(source,"r");
	FILE *outptr = NULL;

	if (output == NULL && !individual)
		outptr = stdout;
	else if (output != NULL && !individual)
		outptr = fopen(output,"w");

	if (ptr != NULL)
	{
		fgets(buffer,BUFFER_SIZE-1,ptr);

		do {
			char *result = strstr(buffer,delimiters[dindex]);

			if (result != NULL)
			{
				if (pair->segmentName != NULL && strstr(result,pair->segmentName) != NULL)
				{
					dindex  = (dindex+1) % 2;
					found = !found;
					insegment = 1;
				}
				else if (pair->segmentName == NULL)
				{
					dindex = (dindex+1) % 2;
					found = !found;
					insegment = 1;
				}

				if (found && insegment)
				{
					char outputName[1024];
					char *segName = strnext(result);
					char prefix[1024], extension[64];

					memset(outputName,0,sizeof(outputName));
					memset(prefix,0,sizeof(prefix));
					memset(extension,0,sizeof(extension));

					if (ptr != stdin)
					{
						char *fname = basename(source);
						char *ext = strrchr(fname,'.');

						strncpy(extension,ext,sizeof(extension));

						strncpy(prefix,fname,strlen(fname) - strlen(ext));
					}
					else
						strcpy(extension,".txt");

					if (individual)
					{
						if (output != NULL)
							sprintf(outputName,"%s%d%s",output,segcount,extension);
						if (segName == NULL || !strcmp(segName,""))
							sprintf(outputName,"%s%d%s",prefix,segcount,extension);
						else
							sprintf(outputName,"%s%s",segName,extension);

						outptr = fopen(outputName,"w");
					}
				}
			}

			// While In segment, output to outptr
			if (insegment)
				fprintf(outptr,"%s",buffer);

			if (!found && insegment)
			{
				// We are no longer in a segment
				insegment = 0;

				++segcount;

				// If outptr is active, close it, we are done with last segment
				if (outptr != NULL && individual)
				{
					fclose(outptr);
					outptr = NULL;
				}
			}
		} while (fgets(buffer,BUFFER_SIZE-1,ptr) != NULL && segcount < 1);

		if (ptr != stdin)
			fclose(ptr);
		if (outptr != stdout && outptr != NULL)
			fclose(outptr);
	}
	else
		retValue = 127;

	return retValue;
}

// Extract All
int ExtractAll()
{
	int retValue = 0;
	segment_pair *pair = pairs[currentPair];
	char *delimiters[] = { pair->beginMarker, pair->endMarker };
	int dindex = 0, found = 0, insegment = 0, segcount = 0;

	if (segName != NULL)
	{
		pair->segmentName=DynString(segName);
	}

	FILE *ptr = (source == NULL) ? stdin : fopen(source,"r");
	FILE *outptr = NULL;

	if (output == NULL && !individual)
		outptr = stdout;
	else if (output != NULL && !individual)
		outptr = fopen(output,"w");


	char buffer[BUFFER_SIZE];

	if (ptr != NULL)
	{
		fgets(buffer,BUFFER_SIZE-1,ptr);

		do {
			char *result = strstr(buffer,delimiters[dindex]);

			if (result != NULL)
			{
				if (pair->segmentName != NULL && strstr(result,pair->segmentName) != NULL)
				{
					dindex  = (dindex+1) % 2;
					found = !found;
					insegment = 1;
				}
				else if (pair->segmentName == NULL)
				{
					dindex = (dindex+1) % 2;
					found = !found;
					insegment = 1;
				}

				if (found && insegment)
				{
					char outputName[1024];
					char *segName = strnext(result);
					char prefix[1024], extension[64];

					memset(outputName,0,sizeof(outputName));
					memset(prefix,0,sizeof(prefix));
					memset(extension,0,sizeof(extension));

					if (ptr != stdin)
					{
						char *fname = basename(source);
						char *ext = strrchr(fname,'.');

						strncpy(extension,ext,sizeof(extension));

						strncpy(prefix,fname,strlen(fname) - strlen(ext));
					}
					else
						strcpy(extension,".txt");

					if (individual)
					{
						if (output != NULL)
							sprintf(outputName,"%s%d%s",output,segcount,extension);
						else if (segName == NULL || !strcmp(segName,""))
							sprintf(outputName,"%s%d%s",prefix,segcount,extension);
						else
							sprintf(outputName,"%s%s",segName,extension);

						outptr = fopen(outputName,"w");
					} else
						fprintf(outptr,"\n");
				}
			}

			// While In segment, output to outptr
			if (insegment)
				fprintf(outptr,"%s",buffer);

			if (!found && insegment)
			{
				// We are no longer in a segment
				insegment = 0;

				++segcount;

				// If outptr is active, close it, we are done with last segment
				if (outptr != NULL && individual)
				{
					fclose(outptr);
					outptr = NULL;
				}
			}
		} while (fgets(buffer,BUFFER_SIZE-1,ptr) != NULL);

		if (ptr != stdin)
			fclose(ptr);
		if (outptr != stdout && outptr != NULL)
			fclose(outptr);
	}
	else
		retValue = 127;

	return retValue;
}

// Replace Function
int Replace()
{
	int exitValue = 0;

	if (replacement != NULL)
	{
		segment_pair *pair = pairs[currentPair];
		char *delimiters[2] = { pair->beginMarker, pair->endMarker };
		char buffer[BUFFER_SIZE];
		int fd, found = 0, insegment = 0, dindex = 0, replacements = 0;
		FILE *srcptr = NULL, *outptr = NULL;

		if (segName != NULL)
			pair->segmentName = segName;

		if (source == NULL)
			srcptr = stdin;
		else
			srcptr = fopen(source,"r");

		if (output != NULL)
			outptr = fopen(output,"w");
		else
			outptr = stdout;

		fgets(buffer,BUFFER_SIZE-1,srcptr);

		do {
			char *result = strstr(buffer,delimiters[dindex]);

			if (result != NULL && replacements < replacementCount)
			{
				if (pair->segmentName == NULL || (pair->segmentName != NULL && strstr(result,pair->segmentName) != NULL))
				{
					insegment = 1;
					found = !found;
					dindex = (dindex + 1) % 2;

					if (found)
					{
						InsertFile(outptr,replacement);
					}
					else
					{
						++replacements;
					}
				}
			}

			if (!insegment)
				fprintf(outptr,"%s",buffer);

			// insegment must change here, to be sure we account for the segment marker
			insegment = (found && insegment);
		} while (fgets(buffer,BUFFER_SIZE-1,srcptr) != NULL);

		if (srcptr != stdin)
			fclose(srcptr);

		if (outptr != stdout)
			fclose(outptr);
	}
	else
	{
		exitValue = 127;
		fprintf(stderr,"Can't replace anything unless you provide a replacement block\n");
	}

	return exitValue;
}

// List Segments
int List()
{
	FILE *ptr = (source == NULL) ? stdin : fopen(source,"r");

	if (ptr != NULL)
	{
		char buffer[BUFFER_SIZE];

		memset(buffer,0,sizeof(buffer));

		fgets(buffer,BUFFER_SIZE-1,ptr);

		int count=0, line=0;

		do
		{
			if (strstr(buffer,pairs[currentPair]->beginMarker) != NULL)
			{
				fprintf(stdout,"Line %d Item %d - %s\n",line,count,buffer);
				count++;
			}

			++line;
		} while (fgets(buffer,BUFFER_SIZE-1,ptr) != NULL);

		fclose(ptr);
	}
	else
	{
		fprintf(stderr,"Could not open %s for listing\n",source);
		return 1;
	}

	return 0;
}

// Test function
void Test()
{
	fprintf(stderr,"Entering Test Function\n");

	int exitValue = 0;
	char buffer[BUFFER_SIZE];
//	regex_t regex;

	strcpy(buffer,"# [ code-begin SegName ]");
//	exitValue = regcomp(&regex,pairs[currentPair]->beginMarker,0);
//	exitValue = regexec(&regex,buffer,0,NULL,0);

//	fprintf(stderr,"Regex Test %d\n",exitValue);

//	exitValue=0;

//	if (segName != NULL)
//	{
//		pairs[currentPair]->segmentName=DynString(segName);
//	}

	currentPair = 1;

	char *blk = BeginMark(buffer);

	if (blk != NULL)
	{
		char *next = strnext(blk);

		if (next != NULL)
			printf("%s\n",next);
		else
			printf("Nope, didn't work\n");
	}
}

// Main Loop
int main(int argc, char **argv)
{
	int exitCode = 0;

	if (argc == 1)
	{
		Usage();
		exit(127);
	}

	NewTrace("./trace.txt");

	Allocate();

	ProcessArgs(argc,argv);

	if (opMode != test)
	{
		if (currentPair == -1)
			currentPair = FindSegmentDelimiter();

		if (currentPair == -1)
		{
			fprintf(stderr,"Cannot continue, no segment delimiter sepcified and none can be detected\n");
			return (127);
		}
	}

	switch (opMode)
	{
	case extract:
		exitCode = Extract();
		break;
	case replace:
		exitCode = Replace();
		break;
	case extractall:
		exitCode = ExtractAll();
		break;
	case list:
		exitCode = List();
		break;
	case test:
		Test();
		break;
	}

	Deallocate();

	if (removeSource)
		remove(source);

	TraceClose();

	return (exitCode);
}

