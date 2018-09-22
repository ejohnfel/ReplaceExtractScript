CC=gcc
INSTALL=/usr/local/bin

segments: segments.c
	@echo "Making segment runtime"
	@$(CC) -o $@ $<

install: segments
	@[ -e $(INSTALL)/$< -o -L $(INSTALL)/$< ] && sudo rm $(INSTALL)/$<
	@sudo cp $< $(INSTALL)/$<
	@sudo chmod +rx $(INSTALL)/$<

test: segments
	@echo "List Delimiter Sets"
	@./segments -s
	@echo "List segments in test file with delimiter guess"
	@./segments -l -f testfile.txt
	@echo "List segments in test file for delimiter set 0"
	@./segments -l -d 0 -f testfile.txt
	@echo "List Segments in test file for delimiter set 1"
	@./segments -l -d 1 -f testfile.txt
	@echo "List Segments in test file for delimiter set 2"
	@./segments -l -d 2 -f testfile.txt
	@echo "List Segments in test file for delimiter set 3"
	@./segments -l -d 3 -f testfile.txt
	@echo "List Segments in test file for delimiter set 3 which is defined on CmdLine"
	@./segments -l -b "begin-custom" -e "end-custom" -d 3 -f testfile.txt
	@#echo "Testing Extract All"
	@#./segments -a -d 1 -f testfile.txt
	@#echo "Testing Extract With Name"
	@#./segments -e -d 1 -n "ScriptSegment" -f testfile.txt
	@#echo "Testing Extract With Name and Individual Turned On"
	@#./segments -e -d 1 -1 -n "ScriptSegment" -f testfile.txt
	@#echo "Testing Extract Without Name"
	@#./segments -e -d 1 -f testfile.txt
	@#echo "Testing Extract with output name"
	@#./segments -e -d 1 -n "ScriptSegment" -f testfile.txt -o testsegment.txt
	@#echo "Testing with Individual Turned On"
	@#./segments -a -1 -d 1 -f testfile.txt
	@echo "Testing Replacement to stdout ***"
	@./segments -r -d 1 -f testfile.txt -i replacement.txt
	@echo "****"

.PHONY: clean

clean:
	@rm mkme*
	@rm segments
