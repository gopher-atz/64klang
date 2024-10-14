using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPTau : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPTau()
            : base("[tT]au", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "Tau()");

            output.Push((double)(Math.PI*2.0));
        }
    }
}
