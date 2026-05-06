using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPExp : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPExp()
            : base("[eE]xp", false)
        {
        }

        /// <summary>
        /// Execute Exp
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Exp()");
            object top = PopOrGet(output);
            Exp(output, top);
        }

        /// <summary>
        /// Exponent
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Exp(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Exp(v));
            else
                throw new InvalidArgumentTypeException("Exp()", arg);
        }
    }
}
