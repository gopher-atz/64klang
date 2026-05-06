using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPDecimal : MPDataType
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPDecimal()
        {
        }

        /// <summary>
        /// Read a decimal
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <param name="converted"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken, out object converted)
        {
            string sign = @"^";
            if (IsUnary(previousToken))
                sign = @"^[\+\-]?";

            // Match an integer
            Match m = Regex.Match(expression, sign + @"\d+[mM]?(?![\w\.])");
            if (m.Success)
            {
                try
                {
                    converted = Decimal.Parse(m.Value, System.Globalization.CultureInfo.InvariantCulture);
                }
                catch (Exception)
                {
                    converted = null;
                    return -1;
                }
                return m.Length;
            }

            // Default
            converted = null;
            return -1;
        }
    }
}
