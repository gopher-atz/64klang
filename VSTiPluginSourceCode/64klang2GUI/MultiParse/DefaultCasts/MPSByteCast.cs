using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPSByteCast : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPSByteCast()
            : base("(sbyte)", PrecedenceUnary, false)
        {
        }

        /// <summary>
        /// Find byte cast
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            if (!IsUnary(previousToken))
                return -1;
            Match m = Regex.Match(expression, @"^\((sb|SB)yte\)");
            if (m.Success)
                return m.Length;
            return -1;
        }

        /// <summary>
        /// Execute byte cast
        /// </summary>
        /// <param name="output"></param>
        public override void Execute(Stack<object> output)
        {
            // Pop object from the stack
            object top = PopOrGet(output);
            TypeCode tc = Type.GetTypeCode(top.GetType());

            switch (tc)
            {
                case TypeCode.Byte: output.Push((sbyte)(Byte)top); return;
                case TypeCode.Char: output.Push((sbyte)(Char)top); return;
                case TypeCode.Decimal: output.Push((sbyte)(Decimal)top); return;
                case TypeCode.Double: output.Push((sbyte)(Double)top); return;
                case TypeCode.Int16: output.Push((sbyte)(Int16)top); return;
                case TypeCode.Int32: output.Push((sbyte)(Int32)top); return;
                case TypeCode.Int64: output.Push((sbyte)(Int64)top); return;
                case TypeCode.SByte: output.Push((sbyte)(SByte)top); return;
                case TypeCode.Single: output.Push((sbyte)(Single)top); return;
                case TypeCode.UInt16: output.Push((sbyte)(UInt16)top); return;
                case TypeCode.UInt32: output.Push((sbyte)(UInt32)top); return;
                case TypeCode.UInt64: output.Push((sbyte)(UInt64)top); return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("(SByte)", top);
        }
    }
}
