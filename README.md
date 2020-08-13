# RenameByDate
A Windows command line program to rename files in a directory tree using the optional given date parameters in combination with the Date Taken metadata to rename the file. 

If the optional date parameters are given, that date will be used in combination with the time from the Date Taken metadata. In this scenario, a copy of the file will be created in a sub-folder called "Corrected" to contain the new named file with corrected Date Taken based on the command line date information and the pre-existing Date Taken time information.

The pathname can contain wild cards to limit which files will be changed or a specific filename to change a single file.

If a folder name is given, the program will recurse through the sub-folders renaming all of the files found in the directory tree.
