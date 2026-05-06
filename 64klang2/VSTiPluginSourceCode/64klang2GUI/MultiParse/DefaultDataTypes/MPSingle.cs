using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPSingle : MPDataType
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPSingle()
            : base()
        {
        }

        /// <summary>
        /// Find a double
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <param name="convertedToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken, out object convertedToken)
        {
            string sign = @"";
            if (IsUnary(previousToken))
                sign = @"[\+\-]?";

            Match m = Regex.Match(expression, @"^(?<mantissa>" + sign + @"\d+(\.\d+)?)([eE](?<exponent>[\-\+]?\d+))?[fF]?(?![\w\.])");
            if (m.Success)
            {
                try
                {
                    if (m.Groups["exponent"].Success)
                    {
                        // Scientific notation
                        convertedToken = Single.Parse(m.Groups["mantissa"] + "E" + m.Groups["exponent"], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture);
                        return m.Length;
                    }
                    else
                    {
                        // No scientific notation
                        convertedToken = Single.Parse(m.Groups["mantissa"].Value, System.Globalization.CultureInfo.InvariantCulture);
                        return m.Length;
                    }
                }
                catch (Exception)
                {
                    
                    convertedToken = null;
                    return -1;
                }
            }

            // Default
            convertedToken = null;
            return -1;
        }
    }
}
