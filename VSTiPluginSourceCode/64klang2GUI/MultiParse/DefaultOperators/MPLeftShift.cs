using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPLeftShift : MPOperator
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPLeftShift()
            : base("<<", PrecedenceShift, true)
        {
        }

        /// <summary>
        /// Find left shift
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            if (IsUnary(previousToken))
                return -1;
            if (expression.StartsWith("<<"))
                return 2;
            return -1;
        }

        /// <summary>
        /// Execute left shift
        /// </summary>
        /// <param name="output"></param>
        public override void Execute(Stack<object> output)
        {
            // Pop two objects from the stack
            object right = PopOrGet(output);
            object left = PopOrGet(output);
            LeftShift(output, left, right);
        }

        /// <summary>
        /// Left shift
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void LeftShift(Stack<object> output, object left, object right)
        {

            // The right has to be an int
            int shift;
            if (!CastImplicit(right, out shift))
                throw new InvalidOperatorTypesException("<<", left, right);

            // Default implementation depends on the typecodes
            TypeCode tcl = Type.GetTypeCode(left.GetType());
            switch (tcl)
            {
                case TypeCode.Byte: output.Push((Byte)left << shift); return;
                case TypeCode.Char: output.Push((Char)left << shift); return;
                case TypeCode.Int16: output.Push((Int16)left << shift); return;
                case TypeCode.Int32: output.Push((Int32)left << shift); return;
                case TypeCode.Int64: output.Push((Int64)left << shift); return;
                case TypeCode.SByte: output.Push((SByte)left << shift); return;
                case TypeCode.UInt16: output.Push((UInt16)left << shift); return;
                case TypeCode.UInt32: output.Push((UInt32)left << shift); return;
                case TypeCode.UInt64: output.Push((UInt64)left << shift); return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("<<", left, right);
        }
    }
}
