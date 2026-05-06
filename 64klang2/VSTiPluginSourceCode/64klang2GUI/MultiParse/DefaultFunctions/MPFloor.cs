using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPFloor : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPFloor()
            : base("[fF]loor", false)
        {
        }

        /// <summary>
        /// Execute rounding
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // Floor accepts one or 2 parameters
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, 2, "Floor()");
            object top = PopOrGet(output);
            Floor(output, top);
        }

        /// <summary>
        /// Floor
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Floor(Stack<object> output, object arg)
        {
            // The first argument is either a double or decimal
            double dbl;
            decimal dec;
            bool dblok = CastImplicit(arg, out dbl);
            bool decok = CastImplicit(arg, out dec);

            // Check
            if (dblok && decok)
                throw new ParseException("The call to Floor() is ambiguous for the type '" + arg.GetType() + "'");

            if (dblok)
                output.Push(Math.Floor(dbl));
            else if (decok)
                output.Push(Math.Floor(dec));
            else
                throw new InvalidArgumentTypeException("Floor()", arg);
        }
    }
}
