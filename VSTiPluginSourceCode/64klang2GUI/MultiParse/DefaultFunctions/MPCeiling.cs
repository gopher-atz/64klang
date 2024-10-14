using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPCeiling : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPCeiling()
            : base("[cC]eil", false)
        {
        }

        /// <summary>
        /// Execute rounding
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // Ceiling accepts one or 2 parameters
            if (arguments != 1 && arguments != 2)
                throw new InvalidArgumentCountException(1, 2, "Ceil()");
            object top = PopOrGet(output);
            Ceiling(output, top);
        }

        /// <summary>
        /// Ceiling
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Ceiling(Stack<object> output, object arg)
        {
            // The first argument is either a double or decimal
            double dbl;
            decimal dec;
            bool dblok = CastImplicit(arg, out dbl);
            bool decok = CastImplicit(arg, out dec);

            // Check
            if (dblok && decok)
                throw new ParseException("The call to Ceil() is ambiguous for the type '" + arg.GetType() + "'");

            if (dblok)
                output.Push(Math.Ceiling(dbl));
            else if (decok)
                output.Push(Math.Ceiling(dec));
            else
                throw new InvalidArgumentTypeException("Ceil()", arg);
        }
    }
}
