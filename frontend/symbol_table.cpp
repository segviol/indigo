#include "symbol_table.hpp"

using namespace front::symbolTable;

SymbolTable::SymbolTable() {
  holder_symbol = SharedSyPtr(new symbol::VoidSymbol());
}

/*
 * del symbol_table or pop this input symbol all with del input symbol
 *
 * if undefine error happened, we only save the symbol first appeared
 * the appearance of error is that two symbols are pushed with same name and
 * layerNum
 */
void SymbolTable::push_symbol(SharedSyPtr symbol) {
  if (symbol->getLayerNum() !=
      find_least_layer_symbol(symbol->getName())->getLayerNum()) {
    symbol_stack.push_back(symbol);
    symbol_map[symbol->getName()].push_back(symbol);
  }
}

/*
 * this method will del the Symbol object on the top of stack which was
 * allocated by new in push_symbol method
 */
void SymbolTable::pop_symbol() {
  SharedSyPtr symbol = symbol_stack.back();
  symbol_stack.pop_back();
  symbol_map[symbol->getName()].pop_back();
}

/*
 * this method will pop the input layer`s all symbols only when they are on the
 * top of stack
 */
void SymbolTable::pop_layer_symbols(unsigned int layer) {
  while (!symbol_stack.empty() && symbol_stack.back()->getLayerNum() == layer) {
    pop_symbol();
  }
}

bool SymbolTable::empty() const { return symbol_stack.empty(); }

size_t SymbolTable::size() const { return symbol_stack.size(); }

SharedSyPtr SymbolTable::find_least_layer_symbol(string name) {
  if (symbol_map.count(name) == 1 && symbol_map[name].size() >= 1) {
    return symbol_map[name].back();
  } else {
    return holder_symbol;
  }
}
