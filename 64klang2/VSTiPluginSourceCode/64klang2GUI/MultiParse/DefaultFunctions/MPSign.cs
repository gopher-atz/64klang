using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPSign : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPSign()
            : base("[sS]ign", false)
        {
        }

        /// <summary>
        /// Execute sign operator
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // Expects 1 parameter
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Sign()");

            // Pop object from the stack
            object top = PopOrGet(output);
            Sign(output, top);
        }

        /// <summary>
        /// Sign
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Sign(Stack<object> output, object arg)
        {
            TypeCode tc = Type.GetTypeCode(arg.GetType());
            switch (tc)
            {
                case TypeCode.Byte: output.Push(Math.Sign((Byte)arg)); return;
                case TypeCode.Char: output.Push(Math.Sign((Char)arg)); return;
                case TypeCode.Decimal: output.Push(Math.Sign((Decimal)arg)); return;
                case TypeCode.Double: output.Push(Math.Sign((Double)arg)); return;
                case TypeCode.Int16: output.Push(Math.Sign((Int16)arg)); return;
                case TypeCode.Int32: output.Push(Math.Sign((Int32)arg)); return;
                case TypeCode.Int64: output.Push(Math.Sign((Int64)arg)); return;
                case TypeCode.SByte: output.Push(Math.Sign((SByte)arg)); return;
                case TypeCode.Single: output.Push(Math.Sign((Single)arg)); return;
                case TypeCode.UInt16: output.Push(Math.Sign((UInt16)arg)); return;
                case TypeCode.UInt32: output.Push(Math.Sign((UInt32)arg)); return;
            }

            // Invalid operation
            throw new InvalidArgumentTypeException("Sign()", arg);
        }
    }
}
