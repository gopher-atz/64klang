using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPRand : MPFunction
    {

        static Random rand = new Random();

        /// <summary>
        /// Constructor
        /// </summary>
        public MPRand()
            : base("[rR]and", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "Rand()");
            output.Push((double)(rand.NextDouble() * 2.0 - 1.0));
        }
    }
}
