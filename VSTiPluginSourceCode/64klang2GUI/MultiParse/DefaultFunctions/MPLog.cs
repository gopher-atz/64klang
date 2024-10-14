using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPLog : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPLog()
            : base("[lL]og", false)
        {
        }

        /// <summary>
        /// Execute logarithm
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // This function needs two arguments
            object val, b;
            switch (arguments)
            {
                case 1:
                    val = PopOrGet(output);
                    Log(output, val);
                    break;
                case 2:
                    b = PopOrGet(output);
                    val = PopOrGet(output);
                    Log(output, val, b);
                    break;
                default:
                    throw new InvalidArgumentCountException(1, 2, "Log()");
            }
        }

        /// <summary>
        /// Logarithm
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Log(Stack<object> output, object arg)
        {
            double d;
            if (CastImplicit(arg, out d))
                output.Push(Math.Log(d));
            else
                throw new InvalidArgumentTypeException("Log()", arg);
        }

        /// <summary>
        /// Logarithm
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void Log(Stack<object> output, object left, object right)
        {
            double d, b;
            if (CastImplicit(left, out d) && CastImplicit(right, out b))
                output.Push(Math.Log(d, b));
            else
                throw new InvalidArgumentTypeException("Log()", left, right);
        }
    }
}
