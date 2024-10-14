using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MultiParse
{
    public class ParseException : Exception
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="msg"></param>
        public ParseException(string msg)
            : base(msg)
        {
        }
    }

    public class InvalidArgumentCountException : ParseException
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="expected"></param>
        /// <param name="function"></param>
        public InvalidArgumentCountException(int expected, string function)
            : base("Invalid amount of arguments. " + expected + " argument" + (expected > 1 ? "s" : "") + " expected for " + function)
        {
        }

        public InvalidArgumentCountException(int expected1, int expected2, string function)
            : base("Invalid amount of arguments. " + expected1 + " or " + expected2 + " arguments expected for " + function)
        {
        }
    }

    public class InvalidOperatorTypesException : ParseException
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="op"></param>
        /// <param name="a"></param>
        public InvalidOperatorTypesException(string op, object a)
            : base("Operator '" + op + "' cannot be applied to operand of type '" + a.GetType() + "'")
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="op"></param>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public InvalidOperatorTypesException(string op, object a, object b)
            : base("Operator '" + op + "' cannot be applied to operands of type '" + a.GetType() + "' and '" + b.GetType() + "'")
        {
        }
    }

    public class InvalidArgumentTypeException : ParseException
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="function"></param>
        /// <param name="a"></param>
        public InvalidArgumentTypeException(string function, object a)
            : base("Function " + function + " can not execute with argument of type '" + a.GetType() + "'")
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="function"></param>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public InvalidArgumentTypeException(string function, object a, object b)
            : base("Function " + function + " can not execute with arguments of type '" + a.GetType() + "' and '" + b.GetType() + "'")
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="function"></param>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public InvalidArgumentTypeException(string function, object a, object b, object c)
            : base("Function " + function + " can not execute with arguments of type '" + a.GetType() + "' and '" + b.GetType() + "' and '" + c.GetType() + "'")
        {
        }
    }
}
