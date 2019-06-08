#include <fstream>
#include <iostream>
#include <regex>
#include <string>

using namespace std;

int main()
{
    fstream datafile;
    ofstream savefile;
    char temp[1024];
    datafile.open("Entity data.txt", ios::in);
    savefile.open("Entity data(processed).txt", ios::out);
    cmatch result1;
    cmatch result2;
    regex regex1("(0[xX]([0-9a-fA-F]{2}))");
    regex regex2("(0[xX][0-9a-fA-F]{6})");
    while (!datafile.eof())
    {
        datafile.getline(temp, 1024, '\n');
        if (regex_search(temp, result1, regex1) && regex_search(temp, result2, regex2))
            savefile << "const int Entity" << result1.str(2) << "_" << result2.str() << " = " << result2.str() << ";"
                     << endl;
    }
    datafile.close();
    savefile.close();
    return 0;
}
