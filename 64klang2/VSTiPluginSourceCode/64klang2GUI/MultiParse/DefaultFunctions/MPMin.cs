using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPMin : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPMin()
            : base("[mM]in", false)
        {
        }

        /// <summary>
        /// Execute the min function
        /// </summary>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 2)
                throw new InvalidArgumentCountException(2, "Min()");

            // Pop two objects from the stack
            object right = PopOrGet(output);
            object left = PopOrGet(output);
            Min(output, left, right);
        }

        /// <summary>
        /// Minimum
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void Min(Stack<object> output, object left, object right)
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
                        case TypeCode.Byte: output.Push(Math.Min(bt, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(bt, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(bt, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(bt, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(bt, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(bt, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(bt, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(bt, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(bt, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(bt, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(bt, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(bt, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Char:
                    char c = (Char)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(c, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(c, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(c, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(c, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(c, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(c, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(c, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(c, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(c, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(c, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(c, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(c, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Decimal:
                    decimal d = (Decimal)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(d, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(d, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(d, (Decimal)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(d, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(d, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(d, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(d, (SByte)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(d, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(d, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(d, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Double:
                    double dbl = (Double)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(dbl, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(dbl, (Char)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(dbl, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(dbl, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(dbl, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(dbl, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(dbl, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(dbl, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(dbl, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(dbl, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(dbl, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.Int16:
                    short s = (Int16)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(s, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(s, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(s, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(s, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(s, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(s, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(s, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(s, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(s, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(s, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(s, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.Int32:
                    int i = (Int32)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(i, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(i, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(i, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(i, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(i, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(i, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(i, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(i, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(i, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(i, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(i, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.Int64:
                    long l = (Int64)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(l, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(l, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(l, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(l, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(l, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(l, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(l, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(l, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(l, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(l, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(l, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.SByte:
                    sbyte sb = (SByte)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(sb, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(sb, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(sb, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(sb, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(sb, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(sb, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(sb, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(sb, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(sb, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(sb, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(sb, (UInt32)right)); return;
                    }
                    break;

                case TypeCode.Single:
                    float f = (Single)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(f, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(f, (Char)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(f, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(f, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(f, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(f, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(f, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(f, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(f, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(f, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(f, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.UInt16:
                    ushort us = (UInt16)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(us, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(us, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(us, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(us, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(us, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(us, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(us, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(us, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(us, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(us, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(us, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(us, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.UInt32:
                    uint ui = (UInt32)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(ui, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(ui, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(ui, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(ui, (Double)right)); return;
                        case TypeCode.Int16: output.Push(Math.Min(ui, (Int16)right)); return;
                        case TypeCode.Int32: output.Push(Math.Min(ui, (Int32)right)); return;
                        case TypeCode.Int64: output.Push(Math.Min(ui, (Int64)right)); return;
                        case TypeCode.SByte: output.Push(Math.Min(ui, (SByte)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(ui, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(ui, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(ui, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(ui, (UInt64)right)); return;
                    }
                    break;

                case TypeCode.UInt64:
                    ulong ul = (UInt64)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(Math.Min(ul, (Byte)right)); return;
                        case TypeCode.Char: output.Push(Math.Min(ul, (Char)right)); return;
                        case TypeCode.Decimal: output.Push(Math.Min(ul, (Decimal)right)); return;
                        case TypeCode.Double: output.Push(Math.Min(ul, (Double)right)); return;
                        case TypeCode.Single: output.Push(Math.Min(ul, (Single)right)); return;
                        case TypeCode.UInt16: output.Push(Math.Min(ul, (UInt16)right)); return;
                        case TypeCode.UInt32: output.Push(Math.Min(ul, (UInt32)right)); return;
                        case TypeCode.UInt64: output.Push(Math.Min(ul, (UInt64)right)); return;
                    }
                    break;
            }

            // Invalid operation
            throw new InvalidArgumentTypeException("Min()", left, right);
        }
    }
}
