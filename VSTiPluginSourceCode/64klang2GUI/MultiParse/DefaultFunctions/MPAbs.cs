using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPAbs : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPAbs()
            : base("[aA]bs", false)
        {
        }

        /// <summary>
        /// Execute absolute value
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Abs()");

            // Pop object from the stack
            object top = PopOrGet(output);
            Abs(output, top);
        }

        /// <summary>
        /// Absolute value
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Abs(Stack<object> output, object arg)
        {
            TypeCode tc = Type.GetTypeCode(arg.GetType());
            switch (tc)
            {
                case TypeCode.Byte: output.Push(Math.Abs((Byte)arg)); return;
                case TypeCode.Char: output.Push(Math.Abs((Char)arg)); return;
                case TypeCode.Decimal: output.Push(Math.Abs((Decimal)arg)); return;
                case TypeCode.Double: output.Push(Math.Abs((Double)arg)); return;
                case TypeCode.Int16: output.Push(Math.Abs((Int16)arg)); return;
                case TypeCode.Int32: output.Push(Math.Abs((Int32)arg)); return;
                case TypeCode.Int64: output.Push(Math.Abs((Int64)arg)); return;
                case TypeCode.SByte: output.Push(Math.Abs((SByte)arg)); return;
                case TypeCode.Single: output.Push(Math.Abs((Single)arg)); return;
                case TypeCode.UInt16: output.Push(Math.Abs((UInt16)arg)); return;
                case TypeCode.UInt32: output.Push(Math.Abs((UInt32)arg)); return;
            }

            // Invalid operation
            throw new InvalidArgumentTypeException("Abs()", arg);
        }
    }
}
