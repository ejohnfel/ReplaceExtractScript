#!/bin/bash

######################################
# Replace Script :
#
# Replace segments of a file using markers
#
######################################

# [ code-begin ExtractSegment ]

#
# Extract Segment : Extract a segment of a file between two markers
#
# Input Parameters
# file-to-search output-name begin-marker end-marker
#
# ExtractSegment ~/.bashrc ./output.txt "begin-marker-expression" "end-marker-expression"
#
function ExtractSegment()
{
	grep "$3" "$1" >> /dev/null

	if [ $? = 0 ]; then
		if [ "$2" = "stdout" ]; then
			sed -n "/$3/,/$4/p" $1 
		else
			sed -n "/$3/,/$4/p" $1 > "$2"
		fi
	else
		msg="Could not file begin marker ($3) in file ($1), can't continue."
		echo -e "${msg}"
		logger -p user.warning -t "ExtractSegment" "${msg}"
		return 127
	fi

	return 0
}

# [ code-end ExtractSegment ]

# [ code-begin ReplaceSegment ]

#
# ReplaceSegment : Replace a segment within a file
#
# Input Parameters
# file-to-edit file-of-new-segment begin-marker end-marker
#
# ReplaceSegment ~/.bashrc ~/projects/detectnat/detectnat.seg "code-begin-expression" "code-end-expression"
#
function ReplaceSegment()
{
	PREFIX=/tmp/prefix.${RANDOM}
	POSTFIX=/tmp/postfix.${RANDOM}

	grep  "$3" "$1" >> /dev/null

	if [ $? = 0 ]; then
		# Backup file
		cp "$1" /tmp
		# Get lines 1 thru start marker - 1
		STARTSEG=`sed -n "/$3/=" "$1"`
		STARTSEG=$((${STARTSEG} -  1))
		# Extract the "pre" part
		sed -n "1,${STARTSEG}p" "$1" > ${PREFIX}

		if [ ! "$4" = "" ]; then
			# Get lines end marker + 1
			ENDSEG=`sed -n "/$4/=" "$1"`
			ENDSEG=$((${ENDSEG} + 1))
			# Extract the "post" part
			sed -n "${ENDSEG},//p" "$1" > ${POSTFIX}

			# Glue the parts together
			cat ${PREFIX} "$2" ${POSTFIX} > "$1"

			# Remove the intermediates
			rm ${PREFIX} ${POSTFIX}
		else
			# Combine prefix and replacement segment
			cat ${PREFIX} "$2" > "$1"

			# Remove intermediate file
			rm ${PREFIX}
		fi
	else
		echo -e "[== Begin Marker ($3) not present in ($1), can't continue."
		logger -p user.warning -t "ReplaceSegment" "Could not fine $3 marker in file $1"
	fi
}

# [ code-end ReplaceSegment ]

function Usage()
{
	echo -e "Replace Segement Script"
	echo -e "======================="
	echo -e "-r\tReplace segment\n\treplace.sh -r src-file replace-file begin-block-expression end-block-expression\n\treplace.sh -r ~/.bashrc myseg.txt \"[ code-begin ]\" \"[ code-end ]\""
	echo -e "-e\tExtract segment\t\treplace.sh -e src-file output-filename begin-block-expression end-block-expression (-e is optional)\n\treplace.sh ~/.bashrc output.txt \"[ code-begin ]\" \"[ code-end ]\""
	echo -e "-h\tThis Usage Menu"
	echo -e "-n"
	echo -e "For extracting, if you want the extract to go to stdout, use the keywords 'stdout' as the output-filename"
}

case $1 in
"-h")
	Usage ;;
"-r")
	MODE=replace
	SRC=$2
	SEG=$3
	BEGINSEG=$4
	ENDSEG=$5 
	;;
"-e")
	MODE=extract
	SRC=$2
	DST=$3
	BEGINSEG=$4
	ENDSEG=$5
	;;
*)
	MODE=EXTRACT
	SRC=$1
	DST=$2
	BEGINSEG=$3
	ENDSEG=$4
	;;
esac

case ${MODE} in
"replace")
	ReplaceSegment "${SRC}" "${SEG}" "${BEGINSEG}" "${SENDSEG}"
	;;
"extract")
	ExtractSegment "${SRC}" "${DST}" "${BEGINSEG}" "${ENDSEG}"
	;;
esac
