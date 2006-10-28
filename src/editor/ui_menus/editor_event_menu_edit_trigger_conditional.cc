/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "editorinteractive.h"
#include "editor_event_menu_edit_trigger_conditional.h"
#include "error.h"
#include "graphic.h"
#include "i18n.h"
#include "map.h"
#include "map_trigger_manager.h"
#include "ui_button.h"
#include "ui_listselect.h"
#include "ui_modal_messagebox.h"
#include "ui_textarea.h"
#include "ui_window.h"
#include "trigger.h"
#include "trigger_conditional.h"
#include "util.h"

Editor_Event_Menu_Edit_TriggerConditional::Editor_Event_Menu_Edit_TriggerConditional
(Editor_Interactive* parent, TriggerConditional* cond, EventChain* chain)
:
UIWindow(parent, 0, 0, 465, 340, _("Edit Trigger Conditional").c_str()),
m_parent(parent),
m_given_cond(cond),
m_event_chain(chain)
{

   // Caption
   UITextarea* tt=new UITextarea(this, 0, 0, _("Edit Trigger Conditional Menu"), Align_Left);
   tt->set_pos((get_inner_w()-tt->get_w())/2, 5);

   const int offsx=5;
   const int offsy=25;
   const int spacing=5;
   int posx=offsx;
   int posy=offsy;
   const int ls_width = 180;

   // Trigger List
   new UITextarea(this, posx, offsy, _("Trigger Conditional: "), Align_Left);
	m_construction= new UIListselect<TriggerConditional_Factory::Token &>(this, spacing, offsy+20, ls_width, get_inner_h()-offsy-55);
   m_construction->selected.set(this, &Editor_Event_Menu_Edit_TriggerConditional::cs_selected);
   m_construction->double_clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::cs_double_clicked);
   posx += ls_width + spacing;

   posy = 35;
	UIButton* b = new UIButton(this, posx, posy, 80, 20, 0, TriggerConditional_Factory::LPAREN);
   b->set_title("(");
   b->clickedid.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_operator);
   posy += 20 + spacing;
	b = new UIButton(this, posx, posy, 80, 20, 0, TriggerConditional_Factory::RPAREN);
   b->set_title(")");
   b->clickedid.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_operator);
   posy += 20 + spacing;
	b = new UIButton(this, posx, posy, 80, 20, 0, TriggerConditional_Factory::AND);
   b->set_title("AND");
   b->clickedid.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_operator);
   posy += 20 + spacing;
	b = new UIButton(this, posx, posy, 80, 20, 0, TriggerConditional_Factory::OR);
   b->set_title("OR");
   b->clickedid.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_operator);
   posy += 20 + spacing;
	b = new UIButton(this, posx, posy, 80, 20, 0, TriggerConditional_Factory::XOR);
   b->set_title("XOR");
   b->clickedid.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_operator);
   posy += 20 + spacing;
	b = new UIButton(this, posx, posy, 80, 20, 0, TriggerConditional_Factory::NOT);
   b->set_title("NOT");
   b->clickedid.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_operator);
   posy += 20 + spacing + spacing;
   b = new UIButton(this, posx, posy, 80, 20, 0);
   b->set_title("<-");
   b->clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_ins_trigger);
   posy += 20 + spacing + spacing;
   b->set_enabled( false );
   m_insert_btn = b;
   b = new UIButton(this, posx, posy, 80, 20, 0);
   b->set_title(_("Delete").c_str());
   b->clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_del_trigger);
   b->set_enabled( false );
   m_delete_btn = b;
   posy += 20 + spacing + spacing + spacing;

   b = new UIButton(this, posx+5, posy, 24, 24, 0);
   b->set_pic(g_gr->get_picture( PicMod_UI, "pics/scrollbar_up.png"));
   b->clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_move_up);
   b->set_enabled( false );
   m_mvup_btn = b;
   b = new UIButton(this, posx+51, posy, 24, 24, 0);
   b->set_pic(g_gr->get_picture( PicMod_UI, "pics/scrollbar_down.png"));
   b->clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_move_down);
   b->set_enabled( false );
   m_mvdown_btn = b;
   posy += 24 + spacing + spacing;

   posx += 80 + spacing;
   new UITextarea(this, posx, offsy, _("Available Triggers: "), Align_Left);
   m_trigger_list=new UIListselect<Trigger &>(this, posx, offsy+20, ls_width, get_inner_h()-offsy-55);
   m_trigger_list->selected.set(this, &Editor_Event_Menu_Edit_TriggerConditional::tl_selected);
   m_trigger_list->double_clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::tl_double_clicked);
	const MapTriggerManager & mtm = parent->get_egbase()->get_map()->get_mtm();
	const MapTriggerManager::Index nr_triggers = mtm.get_nr_triggers();
	for (MapTriggerManager::Index i = 0; i < nr_triggers; ++i) {
		Trigger & tr = mtm.get_trigger_by_nr(i);
		m_trigger_list->add_entry(tr.get_name(), tr);
   }
   m_trigger_list->sort();

   posy=get_inner_h()-30;
   posx=(get_inner_w()/2)-80-spacing;
   b=new UIButton(this, posx, posy, 80, 20, 0);
   b->set_title(_("Ok").c_str());
   b->clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_ok);
   posx=(get_inner_w()/2)+spacing;
   b=new UIButton(this, posx, posy, 80, 20, 1);
   b->set_title(_("Cancel").c_str());
	b->clicked.set(this, &Editor_Event_Menu_Edit_TriggerConditional::clicked_cancel);

   // Add conditional
   if( cond ) {
      std::vector< TriggerConditional_Factory::Token >* tokens = cond->get_infix_tokenlist( );
      for( uint i = 0; i < tokens->size(); i++) {
         TriggerConditional_Factory::Token & t = *new TriggerConditional_Factory::Token( (*tokens)[i] );
	      assert(t.token <= TriggerConditional_Factory::TRIGGER);
	      m_construction->add_entry
		      (t.token == TriggerConditional_Factory::TRIGGER ?
		       static_cast<Trigger * const>(t.data)->get_name()
		       :
		       TriggerConditional_Factory::operators[t.token],
		       t,
		       true);
      }
   }

   center_to_parent();
}

/*
 * cleanup
 */
Editor_Event_Menu_Edit_TriggerConditional::~Editor_Event_Menu_Edit_TriggerConditional(void) {
}

/*
 * Handle mouseclick
 *
 * we're a modal, therefore we can not delete ourself
 * on close (the caller must do this) instead
 * we simulate a cancel click
 */
bool Editor_Event_Menu_Edit_TriggerConditional::handle_mouseclick
(const Uint8 btn, const bool down, int, int)
{
	if (btn == SDL_BUTTON_RIGHT and down) {clicked_cancel(); return true;}
	else return false; // we're not dragable
}

/*
 * a button has been clicked
 */
void Editor_Event_Menu_Edit_TriggerConditional::clicked_cancel() {end_modal(0);}


void Editor_Event_Menu_Edit_TriggerConditional::clicked_ok() {
      // construct token list
      std::vector<TriggerConditional_Factory::Token> tok;

      for(uint i = 0; i < m_construction->get_nr_entries(); i++)
         tok.push_back(m_construction->get_entry(i));

      try {
         if( !tok.size() ) throw TriggerConditional_Factory::SyntaxError();
         TriggerConditional* cond = TriggerConditional_Factory::create_from_infix( m_event_chain, tok );
         assert( cond );
         m_given_cond = cond;
         end_modal( 1 );
      } catch(  TriggerConditional_Factory::SyntaxError err ) {
         UIModal_Message_Box* mb = new UIModal_Message_Box(m_parent, _("Syntax Error"), _("Your conditional contains at least one syntax error. Please correct!\n"),
               UIModal_Message_Box::OK);
         mb->run();
         delete mb;
         return;
      }
}


void Editor_Event_Menu_Edit_TriggerConditional::clicked_operator(int i) {
	assert(i > 0);
	assert(i <= TriggerConditional_Factory::TRIGGER);
      TriggerConditional_Factory::Token & t = *new TriggerConditional_Factory::Token();
      t.data = 0;
	m_construction->add_entry(TriggerConditional_Factory::operators[i], t, true);
}


void Editor_Event_Menu_Edit_TriggerConditional::clicked_ins_trigger() {
      Trigger & trigger = m_trigger_list->get_selection();
      TriggerConditional_Factory::Token & t = *new TriggerConditional_Factory::Token();
      t.data = &trigger;
      t.token = TriggerConditional_Factory::TRIGGER;
      m_construction->add_entry(trigger.get_name(), t, true);
   }


void Editor_Event_Menu_Edit_TriggerConditional::clicked_del_trigger() {
      delete &m_construction->get_selection();

      m_construction->remove_entry( m_construction->get_selection_index());
      m_mvup_btn->set_enabled( false );
      m_mvdown_btn->set_enabled( false );
      m_delete_btn->set_enabled( false );
}


void Editor_Event_Menu_Edit_TriggerConditional::clicked_move_up() {
	assert(m_construction->has_selection()); //  Button should be disabled.
	const int n = m_construction->get_selection_index();
	assert(n > 0); //  Button should be disabled.
	m_construction->switch_entries(n, n - 1);
}
void Editor_Event_Menu_Edit_TriggerConditional::clicked_move_down() {
	assert(m_construction->has_selection()); //  Button should be disabled.
	const uint n = m_construction->get_selection_index();
	assert(n + 1 < m_construction->get_nr_entries()); //  Button should be disabled.
	m_construction->switch_entries(n, n + 1);
}

/*
 * the listbox got selected
 */
void Editor_Event_Menu_Edit_TriggerConditional::tl_selected(int) {
   m_insert_btn->set_enabled( true );
}
void Editor_Event_Menu_Edit_TriggerConditional::cs_selected(int i) {
	assert(i >= 0);
	m_mvdown_btn->set_enabled
		(static_cast<const uint>(i) + 1 < m_construction->get_nr_entries());
	m_mvup_btn->set_enabled(i > 0);
   m_delete_btn->set_enabled( true );
}

/*
 * listbox got double clicked
 */
void Editor_Event_Menu_Edit_TriggerConditional::tl_double_clicked(int)
{clicked_ins_trigger();}
void Editor_Event_Menu_Edit_TriggerConditional::cs_double_clicked(int)
{clicked_del_trigger();}
