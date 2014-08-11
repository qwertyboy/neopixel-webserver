%{
Nathan Duprey
08/10/2014
arduinoFormat.m

This program is intended to read an input HTML file and produce a formatted
text file that can be copied and pasted into an Arduino sketch running as a
webserver. This program asks for the name of the file to be formatted, as
well as the object name of the EthernetClient class being used.
%}

clear;
clc;

file = input('Enter file name to format: ','s');
clientName = input('Enter the name of client object: ','s');
rmString = cat(2,clientName,'.println("");');

%Open the specified file and put each line in a cell array
inFID = fopen(file);
tline = fgetl(inFID);
i = 1;
while ischar(tline)
    text{i,1} = strtrim(tline);
    i = i + 1;
    tline = fgetl(inFID);
end

fclose(inFID);

%format each line of the array
j = 1;
for i = 1 : length(text)
    %make the text say what we want
    repQuote = strrep(text{i,1},'"','''');
    catText = cat(2,clientName,'.println("',repQuote,'");');
    
    %do not place any blank print functions
    if ~strcmpi(catText,rmString)
        fmtText{j,1} = catText;
        j = j + 1;
    end
end

%now ceate a text file with the formatted output for ease of use
fileName = cat(2,'formatted ',file,'.txt');
outFID = fopen(fileName,'w');
for i = 1 : length(fmtText)
    fprintf(outFID,'%s\r\n',fmtText{i,1});
end
fclose(outFID);

fprintf('Output file named "%s" created in current directory.\n',fileName);