using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPTruncate : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPTruncate()
            : base("[tT]runcate", false)
        {
        }

        /// <summary>
        /// Execute truncate
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // Expects one argument
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Truncate()");
            object top = PopOrGet(output);
            Truncate(output, top);
        }

        /// <summary>
        /// Truncate
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Truncate(Stack<object> output, object arg)
        {
            // Try to convert to either decimal or double
            double dbl;
            decimal dec;
            bool dblok = CastImplicit(arg, out dbl);
            bool decok = CastImplicit(arg, out dec);
            
            // Ambiguous call
            if (dblok && decok)
                throw new ParseException("Ambiguous call to Truncate() for type '" + arg.GetType() + "'");
            if (dblok)
                output.Push(Math.Truncate(dbl));
            else if (decok)
                output.Push(Math.Truncate(dec));
            else
                throw new InvalidArgumentTypeException("Truncate()", arg);
        }
    }
}
