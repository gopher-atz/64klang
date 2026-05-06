using System;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    /// <summary>
    /// A class able to parse integers
    /// </summary>
    public class MPInt32 : MPDataType
    {
        /// <summary>
        /// Matches an integer at the start of an expression and removes it afterwards
        /// </summary>
        /// <param name="expression">The expression that possibly starts with a token representing an integer</param>
        /// <param name="previousToken">The last parsed token</param>
        /// <param name="converted">Contains the converted token</param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken, out object converted)
        {
            string sign = @"^";
            if (IsUnary(previousToken))
                sign = @"^[\+\-]?";

            // Match an integer
            Match m = Regex.Match(expression, sign + @"\d+(?![\w\.])");
            if (m.Success)
            {
                try
                {
                    converted = Int32.Parse(m.Value);
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
