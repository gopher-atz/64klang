using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPMax : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPMax()
            : base("[mM]ax", false)
        {
        }

        /// <summary>
        /// Execute the max function
        /// </summary>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 2)
                throw new InvalidArgumentCountException(2, "Max()");

            // Pop two objects from the stack
            object right = PopOrGet(output);
            object left = PopOrGet(output);
            Max(output, left, right);
        }

        /// <summary>
        /// Maximum value
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void Max(Stack<object> output, object left, object right)
        {
            // Default implementation depends on the typecodes
            TypeCode tcl = Type.GetTypeCode(left.GetType());
            TypeCode tcr = Type.GetTypeCode(right.GetType());

            // Depending on the types, calculate the addition
            switch (tcl)
            {

                case TypeCode.Byte:
                    byte bt = (Byte)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(bt, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(bt, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(bt, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(bt, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(bt, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(bt, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(bt, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(bt, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(bt, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(bt, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(bt, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(bt, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Char:
                    char c = (Char)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(c, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(c, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(c, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(c, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(c, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(c, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(c, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(c, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(c, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(c, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(c, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(c, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Decimal:
                    decimal d = (Decimal)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(d, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(d, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(d, (Decimal)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(d, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(d, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(d, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(d, (SByte)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(d, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(d, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(d, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Double:
                    double dbl = (Double)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(dbl, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(dbl, (Char)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(dbl, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(dbl, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(dbl, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(dbl, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(dbl, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(dbl, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(dbl, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(dbl, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(dbl, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Int16:
                    short s = (Int16)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(s, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(s, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(s, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(s, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(s, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(s, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(s, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(s, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(s, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(s, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(s, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.Int32:
                    int i = (Int32)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(i, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(i, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(i, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(i, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(i, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(i, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(i, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(i, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(i, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(i, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(i, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.Int64:
                    long l = (Int64)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(l, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(l, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(l, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(l, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(l, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(l, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(l, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(l, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(l, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(l, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(l, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.SByte:
                    sbyte sb = (SByte)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(sb, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(sb, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(sb, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(sb, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(sb, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(sb, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(sb, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(sb, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(sb, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(sb, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(sb, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.Single:
                    float f = (Single)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(f, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(f, (Char)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(f, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(f, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(f, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(f, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(f, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(f, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(f, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(f, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(f, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.UInt16:
                    ushort us = (UInt16)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(us, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(us, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(us, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(us, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(us, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(us, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(us, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(us, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(us, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(us, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(us, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(us, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.UInt32:
                    uint ui = (UInt32)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(ui, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(ui, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(ui, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(ui, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Max(ui, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Max(ui, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Max(ui, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Max(ui, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(ui, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(ui, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(ui, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(ui, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.UInt64:
                    ulong ul = (UInt64)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Max(ul, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Max(ul, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Max(ul, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Max(ul, (Double)right)); return;
                        case TypeCode.Single: output.Push(Math.Max(ul, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Max(ul, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Max(ul, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Max(ul, (UInt64)right)); return;
                    }
                    break;
            }

            // Invalid operation
            throw new InvalidArgumentTypeException("Max()", left, right);
        }
    }
}
