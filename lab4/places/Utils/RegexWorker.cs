using System;
using System.Collections;
using System.Text.RegularExpressions;

namespace Utils;

public class RegexWorker
{
    private readonly Hashtable regexTable;

    public RegexWorker(params Tuple<string, Regex>[] regexes)
    {
        regexTable = [];
        foreach (var regex in regexes)
        {
            regexTable[regex.Item1] = regex.Item2;
        }
    }

    public bool Compare(string type, string cmpString)
    {
        if (regexTable.Contains(type))
        {
            return ((Regex)regexTable[type]).IsMatch(cmpString);
        }

        return false;
    }
}