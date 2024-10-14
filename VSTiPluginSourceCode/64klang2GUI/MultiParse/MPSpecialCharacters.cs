using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MultiParse
{
    /// <summary>
    /// A special class signifiying an opening round bracket
    /// </summary>
    public class BracketOpen
    {
        public int Match(string expression, object previousToken)
        {
            if (expression[0] == '(')
                return 1;
            return -1;
        }
    }

    /// <summary>
    /// A special class signifying a closing round bracket
    /// </summary>
    public class BracketClose
    {
        public int Match(string expression, object previousToken)
        {
            if (expression[0] == ')')
                return 1;
            return -1;
        }
    }

    /// <summary>
    /// A special class signifying a argument seperator symbol
    /// </summary>
    public class FunctionSeparator
    {
        public int Match(string expression, object previousToken)
        {
            if (expression[0] == ',')
                return 1;
            return -1;
        }
    }
}
