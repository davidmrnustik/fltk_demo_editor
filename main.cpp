#include <cerrno>
#include <iostream>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>

int changed = 0;
char filename[FL_PATH_MAX] = "";
Fl_Text_Buffer *textbuf = 0;
char title[FL_PATH_MAX];
int num_windows = 0;

// width of line number display, if enabled
const int line_num_width = 75;

Fl_Window* new_view();
void save_cb();
void saveas_cb();
void find2_cb(Fl_Widget*, void*);
void replall_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*);
void replcan_cb(Fl_Widget*, void*);
int check_save();
void save_file(char *newfile);
void set_title(Fl_Window *w);
void update_position(int pos, void *v);
void cut_cb(Fl_Widget*, void *v);
void copy_cb(Fl_Widget*, void *v);
void paste_cb(Fl_Widget*, void *v);
void delete_cb(Fl_Widget*, void *v);
void quit_cb(Fl_Widget*, void *v);
void open_cb(Fl_Widget*, void *v);
void load_file(const char *newfile, int ipos);
void insert_cb(Fl_Widget*, void *v);
void view_cb(Fl_Widget*, void *v);
void close_cb(Fl_Widget*, void *v);
void changed_cb(int, int nInserted, int nDeleted, int, const char*, void* v);
void linenumbers_cb(Fl_Widget *w, void* v);
void wordwrap_cb(Fl_Widget *w, void* v);

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int w, int h, const char *t);

    ~EditorWindow();

    Fl_Window *replace_dlg;
    Fl_Input *replace_find;
    Fl_Input *replace_with;
    Fl_Button *replace_all;
    Fl_Return_Button *replace_next;
    Fl_Button *replace_cancel;
    Fl_Text_Editor *editor;
    char search[256];
    int line_numbers;
    int wrap_mode;
};

EditorWindow::EditorWindow(int w, int h, const char *t) : Fl_Double_Window(w, h, t) {
    replace_dlg = new Fl_Window(300, 105, "Replace");

    replace_find = new Fl_Input(70, 10, 200, 25, "Find:");
    replace_find->align(FL_ALIGN_LEFT);

    replace_with = new Fl_Input(70, 40, 200, 25, "Replace:");
    replace_with->align(FL_ALIGN_LEFT);

    replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
    replace_all->callback((Fl_Callback *) replall_cb, this);

    replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
    replace_next->callback((Fl_Callback *) replace2_cb, this);

    replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");
    replace_cancel->callback((Fl_Callback *) replcan_cb, this);

    replace_dlg->end();
    replace_dlg->set_non_modal();

    editor = 0;
    *search = (char)0;
    line_numbers = 0;
    wrap_mode = 0;
};

EditorWindow::~EditorWindow() {
    delete replace_dlg;
}

void set_title(Fl_Window *w) {
    if (filename[0] == '\0') strcpy(title, "Untitled");
    else {
        char *slash;
        slash = strrchr(filename, '/');
#ifdef _WIN32
        if (slash == NULL) slash = strrchr(filename, '\\');
#endif
        if (slash != NULL) strcpy(title, slash + 1);
        else strcpy(title, filename);
    }
    if (changed) strcat(title, " (modified)");

    w->label(title);
}

void update_position(int pos, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    textbuf->select(pos, pos+ strlen(e->search));
    e->editor->insert_position(pos+ strlen(e->search));
    e->editor->show_insert_position();
}

void cut_cb(Fl_Widget*, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    Fl_Text_Editor::kf_cut(0, e->editor);
}

void copy_cb(Fl_Widget*, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    Fl_Text_Editor::kf_copy(0, e->editor);
}

void paste_cb(Fl_Widget*, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    Fl_Text_Editor::kf_paste(0, e->editor);
}

void delete_cb(Fl_Widget*, void *v) {
    textbuf->remove_selection();
}

void new_cb(Fl_Widget*, void *v) {
    if (!check_save()) return;

    filename[0] = '\0';
    textbuf->select(0, textbuf->length());
    textbuf->remove_selection();
    changed = 0;
    textbuf->call_modify_callbacks();
}

void open_cb(Fl_Widget*, void *v) {
    if (!check_save()) return;
    Fl_Native_File_Chooser fnfc;
    fnfc.title("Open file");
    fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (fnfc.show()) return;
    load_file(fnfc.filename(), -1);
}

int loading = 0;
void load_file(const char *newfile, int ipos) {
    loading = 1;
    int insert = (ipos != -1);
    changed = insert;
    if (!insert) strcpy(filename, "");
    int r;
    if (!insert) r = textbuf->loadfile(newfile);
    else r = textbuf->insertfile(newfile, ipos);
    changed = changed || textbuf->input_file_was_transcoded;
    if (r)
        fl_alert("Error reading from file \'%s\': \n%s.", newfile, strerror(errno));
    else
        if (!insert) strcpy(filename, newfile);
    loading = 0;
    textbuf->call_modify_callbacks();
}

void insert_cb(Fl_Widget*, void *v) {
    if (!check_save()) return;
    Fl_Native_File_Chooser fnfc;
    fnfc.title("Insert file");
    fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (fnfc.show()) return;
    EditorWindow *w = (EditorWindow*)v;
    load_file(fnfc.filename(), w->editor->insert_position());
}

void save_cb() {
    if (filename[0] == '\0') {
        // No filename - get one!
        saveas_cb();
        return;
    } else save_file(filename);
}

void saveas_cb() {
    Fl_Native_File_Chooser fnfc;
    fnfc.title("Save File As?");
    fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    if (fnfc.show()) return;
    save_file((char*)fnfc.filename());
}

void view_cb(Fl_Widget*, void *v) {
    Fl_Window *w = new_view();
    w->show();
}

void close_cb(Fl_Widget*, void *v) {
    EditorWindow *w = (EditorWindow*)v;

    if (num_windows == 1) {
        if (!check_save())
            return;
    }

    w->hide();
    w->editor->buffer(0);
    textbuf->remove_modify_callback(changed_cb, w);
    Fl::delete_widget(w);

    num_windows--;
    if (!num_windows) exit(0);
}

void quit_cb(Fl_Widget*, void *v) {
    if (changed && !check_save())
        return;
    exit(0);
}

void undo_cb(Fl_Widget*, void *v) {

}

void find_cb(Fl_Widget* w, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    const char *val;

    val = fl_input("Search String:", e->search);
    if (val != NULL) {
        // User entered a string - got find it!
        strcpy(e->search, val);
        find2_cb(w, v);
    }
}

void find2_cb(Fl_Widget *w, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    if (e->search[0] == '\0') {
        // Search string is blank; get a new one...
        find_cb(w, v);
        return;
    }
    int pos = e->editor->insert_position();
    int found = textbuf->search_forward(pos, e->search, &pos);
    if (found) {
        // Found a match; select and update the position...
        update_position(pos, v);
    } else {
        int found_backwards = textbuf->search_backward(pos, e->search, &pos);
        if (found_backwards)
            update_position(pos, v);
        else
            fl_alert("No occurences of \'%s\' found!", e->search);
    }
}

void replace_cb(Fl_Widget*, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    e->replace_dlg->show();
}

void replall_cb(Fl_Widget*, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') {
        // Search string is blank, get a new one...
        e->replace_dlg->show();
        return;
    }

    e->replace_dlg->hide();

    e->editor->insert_position(0);
    int times = 0;

    // Loop through the whole string
    for (int found = 1; found;) {
        int pos = e->editor->insert_position();
        found = textbuf->search_forward(pos, find, &pos, 1);

        if (found) {
            // Found a match, update the position and replace text;
            textbuf->select(pos, pos + (int) strlen(find));
            textbuf->remove_selection();
            textbuf->insert(pos, replace);
            e->editor->insert_position(pos + (int)strlen(replace));
            e->editor->show_insert_position();
            times++;
        }
    }
    if (times) fl_message("Replaced %d occurrences.", times);
    else fl_alert("No occurrences of \'%s\' found!", find);
}

void replcan_cb(Fl_Widget*, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    e->replace_dlg->hide();
}

void replace2_cb(Fl_Widget*, void *v) {
    EditorWindow *e = (EditorWindow*)v;
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') {
        e->replace_dlg->show();
        return;
    }

    e->replace_dlg->hide();

    e->editor->insert_position(0);

    int pos = e->editor->insert_position();
    int found = textbuf->search_forward(pos, find, &pos, 1);

    if (found) {
        textbuf->select(pos, pos + (int)strlen(find));
        textbuf->remove_selection();
        textbuf->insert(pos, replace);
        textbuf->select(pos, pos + (int)strlen(replace));
        e->editor->insert_position(pos + (int)strlen(replace));
        e->editor->show_insert_position();
    }
    else fl_alert("No occurrences of \'%s\' found!", find);

}

void changed_cb(int, int nInserted, int nDeleted, int, const char*, void* v) {
    if (nInserted || nDeleted) changed = 1;
    EditorWindow *w = (EditorWindow *)v;
    set_title(w);
    if (loading) w->editor->show_insert_position();
}

int check_save() {
    if (!changed) return 1;

    int r = fl_choice("The current file has not been saved.\n"
                      "Would you like to save it now?",
                      "Cancel", "Save", "Discard");

    if (r == 1) {
        save_cb(); // Save the file...
        return !changed;
    }

    return (r == 2) ? 1 : 0;
}

void save_file(char *newfile) {
    if (textbuf->savefile(newfile))
        fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
    else
        strcpy(filename, newfile);
    changed = 0;
    textbuf->call_modify_callbacks();
}

void linenumbers_cb(Fl_Widget *w, void* v) {
    EditorWindow *e = (EditorWindow*)v;
    Fl_Menu_Bar *m = (Fl_Menu_Bar*)w;
    const Fl_Menu_Item *i = m->mvalue();
    if (i->value()) {
        e->editor->linenumber_width(line_num_width); // enable
        e->editor->linenumber_size(e->editor->textsize());
    } else {
        e->editor->linenumber_width(0); // disable
    }
    e->line_numbers = (i->value() ? 1 : 0);
    e->redraw();
}

void wordwrap_cb(Fl_Widget *w, void* v) {
    EditorWindow *e = (EditorWindow*)v;
    Fl_Menu_Bar *m = (Fl_Menu_Bar*)w;
    const Fl_Menu_Item *i = m->mvalue();
    if (i->value())
        e->editor->wrap_mode(Fl_Text_Editor::WRAP_AT_BOUNDS, 0);
    else
        e->editor->wrap_mode(Fl_Text_Editor::WRAP_NONE, 0);
    e->wrap_mode = (i->value() ? 1 : 0);
    e->redraw();
}

Fl_Menu_Item menuitems[] = {
    {"&File", 0, 0, 0, FL_SUBMENU },
    {"&New File", 0, (Fl_Callback *)new_cb },
    {"&Open File", FL_COMMAND + 'o', (Fl_Callback *)open_cb },
    {"&Insert File", FL_COMMAND + 'i', (Fl_Callback *)insert_cb, 0, FL_MENU_DIVIDER },
    {"&Save File", FL_COMMAND + 's', (Fl_Callback *)save_cb },
    {"&Save File &As", FL_COMMAND + FL_SHIFT + 's', (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER },
    {"&New &View", FL_ALT + 'v', (Fl_Callback *)view_cb, 0 },
    {"&Close View", FL_COMMAND + 'w', (Fl_Callback *)close_cb, 0, FL_MENU_DIVIDER },
    {"&Exit", FL_COMMAND + 'q', (Fl_Callback *)quit_cb, 0 },
    { 0 },
    { "&Edit", 0, 0, 0, FL_SUBMENU },
//        { "&Undo", FL_COMMAND + 'z', (Fl_Callback *)undo_cb, 0, FL_MENU_DIVIDER },
    { "Cu&t", FL_COMMAND + 'x', (Fl_Callback *)cut_cb },
    { "&Copy", FL_COMMAND + 'c', (Fl_Callback *)copy_cb },
    { "&Paste", FL_COMMAND + 'v', (Fl_Callback *)paste_cb },
    { "&Delete", 0, (Fl_Callback *)delete_cb },
    { "Preferences",      0, 0, 0, FL_SUBMENU },
    { "Line Numbers",   FL_COMMAND + 'l', (Fl_Callback *)linenumbers_cb, 0, FL_MENU_TOGGLE },
    { "Word Wrap",      0,                (Fl_Callback *)wordwrap_cb, 0, FL_MENU_TOGGLE },
    { 0 },
    { 0 },
    { "&Search", 0, 0, 0, FL_SUBMENU },
    { "&Find...",       FL_COMMAND + 'f', (Fl_Callback *)find_cb },
    { "F&ind Again",    FL_COMMAND + 'g', (Fl_Callback *)find2_cb },
    { "&Replace...",    FL_COMMAND + 'r', (Fl_Callback *)replace_cb },
    { "Re&place Again", FL_COMMAND + 't', (Fl_Callback *)replace2_cb },
    { 0 },
    { 0 }
};

Fl_Window* new_view()
{
    EditorWindow* w = new EditorWindow(640, 370, "Editor");
    w->begin();

    Fl_Menu_Bar *menubar = new Fl_Menu_Bar(0, 0, w->w(), 30);
    menubar->copy(menuitems, w);

//    Fl_Box *box_wordwrap = new Fl_Box(w->w() - 100, 5, 100, 25, "WORD WRAP ON");
//    box_wordwrap->box(FL_NO_BOX);
//    box_wordwrap->show();
//    if (w->wrap_mode)
//        box_wordwrap->show();
//    else
//        box_wordwrap->hide();

    w->editor = new Fl_Text_Editor(0, 30, w->w(), w->h() - 30);
    w->editor->buffer(textbuf);
    w->end();
    w->callback((Fl_Callback *)close_cb, w);

    textbuf->add_modify_callback(changed_cb, w);
    textbuf->call_modify_callbacks();
    num_windows++;

    return w;
}

int main(int argc, char *argv[]) {
    textbuf = new Fl_Text_Buffer;
    Fl_Window *window = new_view();
    window->show(1, argv);

    return Fl::run();
}
