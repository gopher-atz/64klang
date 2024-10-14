using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPMaxTime : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPMaxTime()
            : base("[mM]axtime", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 2)
                throw new InvalidArgumentCountException(2, "MaxTime()");

            // Pop two objects from the stack
            object func = PopOrGet(output);
            object time = PopOrGet(output);
            // if the time is still in limits, push the function again
            if ((double)Expression.Time < (double)time)
                output.Push(func);
            else
                output.Push((double)0.0);
        }
    }
}
