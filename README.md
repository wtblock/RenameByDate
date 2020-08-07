# RenameByDate
A Windows command line program to rename files in a directory tree using the given date parameters and the file's original modified time to insure uniqueness and preserve the sequence of the file. The renamed files are then written into a sub-folder called Corrected with the Date Taken property set to the generated date and time.

The reason I wrote the program is because I scanned some old slides and the scanner did not record the date taken and assigned meaningless names to the images.

When executed without and parameters, some usage information is displayed:

.

RenameByDate, Copyright (c) 2020, by W. T. Block.

.

Usage:

.

.  RenameByDate pathname year month day

.

Where:

.

.  pathname is the root of the tree to be scanned

.  year is the four digit year the date

.  month is the month of the year (1..12)

.  day is the day of the month (1..31)

.
.The program will error out if the date is invalid,

.  for example day 31 in the month of September, or.  day 29 in a non-leap year.

.

The program will recurse through the directory tree given and rename the files in each folder found and will also create a "Corrected" folder under each folder with corrected Date T
Taken metadate recorded. The files are renamed with the given date parameters and the file's orginal modified time information to keep the names unique and in the orginal sequence.

Here is some sample output from the console:

...\Pictures\1980.01.04 Kassel Germany>cd C:\Users\wtblo\OneDrive\Pictures\1980.01.05 Train to Munich Germany

...\Pictures\1980.01.05 Train to Munich Germany>renamebydate . 1980 1 5

.

Executable pathname: ...\Projects\RenameByDate\x64\Release\RenameByDate.exe

.

.

Given pathname: .

.

.

The date parameters yielded: 1980_01_05_00_00_00

.

...\Pictures\1980.01.05 Train to Munich Germany\PICT0143.JPG

File renamed to: 1980_01_05_00_06_52.JPG

New Date: 1980:01:05 00:06:52

...\Pictures\1980.01.05 Train to Munich Germany\PICT0144.JPG

File renamed to: 1980_01_05_00_07_10.JPG

New Date: 1980:01:05 00:07:10

...\Pictures\1980.01.05 Train to Munich Germany\PICT0145.JPG

File renamed to: 1980_01_05_00_07_18.JPG

New Date: 1980:01:05 00:07:18

...\Pictures\1980.01.05 Train to Munich Germany\PICT0146.JPG

File renamed to: 1980_01_05_00_07_24.JPG

New Date: 1980:01:05 00:07:24

