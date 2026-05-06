using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPLerp : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPLerp()
            : base("[lL]erp", false)
        {
        }

        /// <summary>
        /// Execute the max function
        /// </summary>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 3)
                throw new InvalidArgumentCountException(3, "Lerp()");

            // Pop two objects from the stack
            object t = PopOrGet(output);
            object y = PopOrGet(output);
            object x = PopOrGet(output);
            Lerp(output, x, y, t);
        }

        /// <summary>
        /// Maximum value
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void Lerp(Stack<object> output, object x, object y, object t)
        {
            // Default implementation depends on the typecodes
            TypeCode tx = Type.GetTypeCode(x.GetType());
            TypeCode ty = Type.GetTypeCode(y.GetType());
            TypeCode tt = Type.GetTypeCode(t.GetType());

            if (tx != TypeCode.Double || ty != TypeCode.Double || tt != TypeCode.Double)
                throw new InvalidArgumentTypeException("Lerp()", x, y, t);

            output.Push((double)x + ((double)y-(double)x)*(double)t);
        }
    }
}
