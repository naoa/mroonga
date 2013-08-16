/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013 Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "mrn_condition_converter.hpp"

// for debug
#define MRN_CLASS_NAME "mrn::ConditionConverter"

namespace mrn {
  ConditionConverter::ConditionConverter(grn_ctx *ctx)
    : ctx_(ctx) {
    GRN_TEXT_INIT(&column_name_, 0);
    GRN_VOID_INIT(&value_);
  }

  ConditionConverter::~ConditionConverter() {
    grn_obj_unlink(ctx_, &column_name_);
    grn_obj_unlink(ctx_, &value_);
  }

  void ConditionConverter::convert(const Item *where, grn_obj *expression) {
    MRN_DBUG_ENTER_METHOD();

    if (!where || where->type() != Item::COND_ITEM) {
      DBUG_VOID_RETURN;
    }

    Item_cond *cond_item = (Item_cond *)where;
    List_iterator<Item> iterator(*((cond_item)->argument_list()));
    const Item *sub_item;
    while ((sub_item = iterator++)) {
      switch (sub_item->type()) {
      case Item::FUNC_ITEM:
        {
          const Item_func *func_item = (const Item_func *)sub_item;
          switch (func_item->functype()) {
          case Item_func::EQ_FUNC:
            convert_equal(func_item, expression);
            break;
          default:
            break;
          }
        }
        break;
      default:
        break;
      }
    }

    DBUG_VOID_RETURN;
  }

  void ConditionConverter::convert_equal(const Item_func *func_item,
                                         grn_obj *expression) {
    Item **arguments = func_item->arguments();
    Item *left_item = arguments[0];
    Item *right_item = arguments[1];
    if (left_item->type() == Item::FIELD_ITEM) {
      GRN_BULK_REWIND(&column_name_);
#ifdef MRN_ITEM_HAVE_ITEM_NAME
      Item_name_string *name = &(left_item->item_name);
      GRN_TEXT_PUT(ctx_, &column_name_,
                   name->ptr(), name->length());
#else
      GRN_TEXT_PUTS(ctx_, &column_name_, left_item->name);
#endif
      grn_expr_append_const(ctx_, expression, &column_name_,
                            GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx_, expression, GRN_OP_GET_VALUE, 1);
      grn_obj_reinit(ctx_, &value_, GRN_DB_INT64, 0);
      GRN_INT64_SET(ctx_, &value_, right_item->val_int());
      grn_expr_append_const(ctx_, expression, &value_,
                            GRN_OP_PUSH, 1);
      grn_expr_append_op(ctx_, expression, GRN_OP_EQUAL, 2);
      grn_expr_append_op(ctx_, expression, GRN_OP_AND, 2);
    }
  }
}