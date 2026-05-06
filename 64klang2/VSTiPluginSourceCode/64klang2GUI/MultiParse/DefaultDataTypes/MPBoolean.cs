using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPBoolean : MPDataType
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPBoolean()
            : base()
        {
        }

        /// <summary>
        /// Find a boolean
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <param name="convertedToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken, out object convertedToken)
        {
            // Match true
            if (Regex.IsMatch(expression, @"^[tT]rue(?!\w)"))
            {
                convertedToken = true;
                return 4;
            }

            // Match false
            if (Regex.IsMatch(expression, @"^[fF]alse(?!\w)"))
            {
                convertedToken = false;
                return 5;
            }

            // No match
            convertedToken = null;
            return -1;
        }
    }
}
