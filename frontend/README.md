# 编译器前端

## 一、词法分析

### 1. 终结符Token名对应表

| 序号 | 名称       | 内容       |
| ---- | ---------- | ---------- |
| 1    | IDENFR     | 标识符     |
| 2    | INTCON     | 数值常量   |
| 3    | STRCON     | 字符串常量 |
| 4    | CONSTTK    | const      |
| 5    | INTTK      | int        |
| 6    | VOIDTK     | void       |
| 7    | IFTK       | if         |
| 8    | ELSETK     | else       |
| 9    | WHILETK    | while      |
| 10   | BREAKTK    | break      |
| 11   | CONTINUETK | continue   |
| 12   | RETURNTK   | return     |
| 13   | PLUS       | +          |
| 14   | MINU       | -          |
| 15   | MULT       | *          |
| 16   | DIV        | /          |
| 17   | MOD        | %          |
| 18   | LSS        | <          |
| 19   | LEQ        | <=         |
| 20   | GRE        | >          |
| 21   | GEQ        | >=         |
| 22   | EQL        | ==         |
| 23   | NEQ        | !=         |
| 24   | AND        | &&         |
| 25   | OR         | \|\|       |
| 26   | NOT        | !          |
| 27   | ASSIGN     | =          |
| 28   | SEMICN     | ;          |
| 29   | COMMA      | ,          |
| 30   | LPARENT    | (          |
| 31   | RPARENT    | )          |
| 32   | LBRACK     | [          |
| 33   | RBRACK     | ]          |
| 34   | LBRACE     | {          |
| 35   | RBRACE     | }          |



### 2. 词法分析说明

1. 数值常量包含三种类型：八进制、十进制、十六进制
2. 字符串常量不包含双引号



## 二、语法分析

语法分析采用梯度下降的形式

