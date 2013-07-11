//===- UnaryOp.cpp --------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/Script/UnaryOp.h>
#include <mcld/Script/Operand.h>
#include <cassert>

using namespace mcld;
//===----------------------------------------------------------------------===//
// UnaryOp
//===----------------------------------------------------------------------===//
template<>
IntOperand* UnaryOp<Operator::UNARY_PLUS>::eval()
{
  IntOperand* res = result();
  res->setValue(+ m_pOperand->value());
  return res;
}

template<>
IntOperand* UnaryOp<Operator::UNARY_MINUS>::eval()
{
  IntOperand* res = result();
  res->setValue(- m_pOperand->value());
  return res;
}

template<>
IntOperand* UnaryOp<Operator::LOGICAL_NOT>::eval()
{
  IntOperand* res = result();
  res->setValue(! m_pOperand->value());
  return res;
}

template<>
IntOperand* UnaryOp<Operator::BITWISE_NOT>::eval()
{
  IntOperand* res = result();
  res->setValue(~ m_pOperand->value());
  return res;
}

template<>
IntOperand* UnaryOp<Operator::ABSOLUTE>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::ADDR>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::ALIGNOF>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::DATA_SEGMENT_END>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::DEFINED>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::LENGTH>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::LOADADDR>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::NEXT>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::ORIGIN>::eval()
{
  // TODO
  assert(0);
  return result();
}

template<>
IntOperand* UnaryOp<Operator::SIZEOF>::eval()
{
  // TODO
  assert(0);
  return result();
}
