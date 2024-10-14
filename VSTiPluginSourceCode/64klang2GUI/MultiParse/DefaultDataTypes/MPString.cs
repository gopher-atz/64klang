using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPString : MPDataType
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPString()
            : base()
        {
        }

        /// <summary>
        /// Try to find expression
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <param name="convertedToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken, out object convertedToken)
        {
            // Find brackets
            if (expression.Length < 2 || expression[0] != '\"')
            {
                convertedToken = null;
                return -1;
            }

            StringBuilder sb = new StringBuilder();
            int i = 1;
            for (i = 1; i <= expression.Length; i++)
            {
                // End of string means error
                if (i == expression.Length)
                    throw new ParseException("Quote mismatch, missing a '\"'");

                // Get the next character
                char c = expression[i];
                if (c == '\\')
                {
                    // Escape characters
                    i++;
                    if (i == expression.Length)
                        throw new ParseException("Quote mismatch, missing a '\"'");
                    char esc = expression[i];
                    switch (esc)
                    {
                        case '\'':
                            sb.Append('\'');
                            break;
                        case '\"':
                            sb.Append('\"');
                            break;
                        case '\\':
                            sb.Append('\\');
                            break;
                        case '0':
                            sb.Append('\0');
                            break;
                        case 'a':
                            sb.Append('\a');
                            break;
                        case 'b':
                            sb.Append('\b');
                            break;
                        case 'f':
                            sb.Append('\f');
                            break;
                        case 'n':
                            sb.Append('\n');
                            break;
                        case 'r':
                            sb.Append('\r');
                            break;
                        case 't':
                            sb.Append('\t');
                            break;
                        case 'v':
                            sb.Append('\v');
                            break;
                        case 'u':
                            sb.Append(ReadUTF16(expression, ref i));
                            break;
                        case 'U':
                            sb.Append(ReadUnicodeSurrogatePair(expression, ref i));
                            break;
                        case 'x':
                            sb.Append(ReadUTF16Var(expression, ref i));
                            break;
                        default:
                            throw new ParseException("Unrecognized escape character found for '\\" + esc + "' in " + expression);
                    }
                }
                else
                {
                    // Normal operation
                    if (c == '\"')
                        break;
                    sb.Append(c);
                }
            }

            // Found results
            string result = sb.ToString();
            convertedToken = result;

            // Return the string length = index + 1
            return i + 1;
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
        /// Read a surrogate pair unicode character
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="i"></param>
        /// <returns></returns>
        private string ReadUnicodeSurrogatePair(string expression, ref int i)
        {
            StringBuilder unicode = new StringBuilder();
            for (int k = 0; k < 8; k++)
            {
                i++;
                if (i == expression.Length)
                    throw new ParseException("Quote mismatch, missing a '\"'");
                unicode.Append(expression[i]);
            }
            try
            {
                return Char.ConvertFromUtf32(int.Parse(unicode.ToString(), System.Globalization.NumberStyles.HexNumber));
            }
            catch (Exception)
            {
                throw new ParseException("Unrecognized escape sequence '\\U" + unicode + " for " + expression);
            }
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
