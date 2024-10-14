using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPIfThen : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPIfThen()
            : base("[iI]fthen", false)
        {
        }

        /// <summary>
        /// Execute the max function
        /// </summary>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 3)
                throw new InvalidArgumentCountException(3, "MPIfThen()");

            // Pop two objects from the stack
            object right = PopOrGet(output);
            object left = PopOrGet(output);
            object condition = PopOrGet(output);
            IfThen(output, condition, left, right);
        }

        /// <summary>
        /// Maximum value
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void IfThen(Stack<object> output, object condition, object left, object right)
        {
            // Default implementation depends on the typecodes
            TypeCode tcc = Type.GetTypeCode(condition.GetType());
            TypeCode tcl = Type.GetTypeCode(left.GetType());
            TypeCode tcr = Type.GetTypeCode(right.GetType());

            // Depending on the types, calculate the addition
            switch (tcc)
            {
                case TypeCode.Boolean:
                    bool b = (bool)condition;
                    if (b == true)
                    {
                        switch (tcl)
                        {
                            case TypeCode.Boolean: output.Push((bool)left); return;
                            case TypeCode.Double: output.Push((double)left); return;
                        }
                    }
                    else
                    {
                        switch (tcr)
                        {
                            case TypeCode.Boolean: output.Push((bool)right); return;
                            case TypeCode.Double: output.Push((double)right); return;
                        }                        
                    }
                    break;                
            }

            // Invalid operation
            throw new InvalidArgumentTypeException("IfThen()", condition, left, right);
        }
    }
}
