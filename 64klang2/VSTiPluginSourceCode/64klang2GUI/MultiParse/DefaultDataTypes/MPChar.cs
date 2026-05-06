using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    /// <summary>
    /// A class able to parse (positive) integers
    /// </summary>
    public class MPChar : MPDataType
    {
        /// <summary>
        /// Matches an integer at the start of an expression and removes it afterwards
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <param name="converted"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken, out object converted)
        {
            if (expression.Length < 3)
            {
                converted = null;
                return -1;
            }
            if (expression[0] != '\'')
            {
                converted = null;
                return -1;
            }

            // Detect escape character
            if (expression[1] == '\\')
            {
                int i = 2;
                switch (expression[2])
                {
                    case '\'':
                        converted = '\\';
                        break;
                    case '\"':
                        converted = '\"';
                        break;
                    case '\\':
                        converted = '\\';
                        break;
                    case '0':
                        converted = '\0';
                        break;
                    case 'a':
                        converted = '\a';
                        break;
                    case 'b':
                        converted = '\b';
                        break;
                    case 'f':
                        converted = '\f';
                        break;
                    case 'n':
                        converted = '\n';
                        break;
                    case 'r':
                        converted = '\r';
                        break;
                    case 't':
                        converted = '\t';
                        break;
                    case 'v':
                        converted = '\v';
                        break;
                    case 'u':
                        converted = ReadUTF16(expression, ref i);
                        break;
                    case 'x':
                        converted = ReadUTF16Var(expression, ref i);
                        break;
                    default:
                        converted = null;
                        return -1;
                }

                i++;
                if (i >= expression.Length)
                    throw new ParseException("Quote mismatch, missing a '\"'");

                // Final single quote, return length = index + 1
                if (expression[i] == '\'')
                    return i + 1;
                else
                {
                    converted = null;
                    return -1;
                }
            }

            // Normal case
            converted = expression[1];
            if (expression[2] == '\'')
                return 3;

            // Default
            converted = null;
            return -1;
        }

        /// <summary>
        /// Read a unicode character
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="i"></param>
        /// <returns></returns>
        private char ReadUTF16(string expression, ref int i)
        {
            StringBuilder unicode = new StringBuilder();
            for (int k = 0; k < 4; k++)
            {
                i++;
                if (i == expression.Length)
                    throw new ParseException("Quote mismatch, missing a '\"'");
                unicode.Append(expression[i]);
            }
            return (char)int.Parse(unicode.ToString(), System.Globalization.NumberStyles.HexNumber);
        }

        /// <summary>
        /// Read a unicode character with variable length
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="i"></param>
        /// <returns></returns>
        private char ReadUTF16Var(string expression, ref int i)
        {
            StringBuilder unicode = new StringBuilder();
            for (int k = 0; k < 4; k++)
            {
                i++;
                if (i == expression.Length)
                    throw new ParseException("Quote mismatch, missing a '\"'");
                char c = expression[i];
                if (((c >= 'A') && (c <= 'F'))
                    || ((c >= 'a') && (c <= 'f'))
                    || ((c >= '0') && (c <= '9')))
                    unicode.Append(c);
                else if (k == 0)
                    throw new ParseException("Unrecognized escape sequence '\\x" + c + "' for " + expression);
                else
                {
                    i--;
                    break;
                }
            }
            try
            {
                return (char)int.Parse(unicode.ToString(), System.Globalization.NumberStyles.HexNumber);
            }
            catch (Exception)
            {
                throw new ParseException("Unrecognized escape sequence '\\x" + unicode + "' for " + expression);
            }
        }
    }
}
