# Copyright 2009 Canonical Ltd.
#
# This file is part of desktopcouch.
#
# desktopcouch is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# as published by the Free Software Foundation.
#
# desktopcouch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with desktopcouch.  If not, see <http://www.gnu.org/licenses/>.
#
# Authors: Rick Spencer <rick.spencer@canonical.com>

"""A Treeview for CouchDB"""

import gtk
import gobject
from server import CouchDatabase
from record import Record
import desktopcouch

class CouchGrid(gtk.TreeView):

    def __init__(self, database_name, record_type=None, keys=None, uri=None,
            ctx=desktopcouch.local_files.DEFAULT_CONTEXT):
        """Create a new Couchwidget
        arguments:
        database_name - specify the name of the database in the desktop
        couchdb to use. If the specified database does not exist, it
        will be created.

        optional arguments:
        record_type - a string to specify the record_type to use in
        retrieving and creating records. Note that if no records exist
        in the CouchDB then the keys argument must also be used or
        a RuntimeError will result.

        keys - a list of strings specifying keys to use in
        the columns of the CouchGrid. The keys will also be used for the
        column titles. To change the Titles, use set_title() for each
        column.

        Note that the CouchGrid uses keys for keys, and visa versa.
        If a record does not contain a value for a specified value
        the CouchGrid will simply display an empty string for the
        value. If the widget is set to editable, the user will be able
        to add values to the database.

        """

        gtk.TreeView.__init__(self)

        #set up the default values
        if type(database_name) is not type(str()):
            raise TypeError("database_name is required and must be a string")

        #set up default values
        self.__record_type = None
        self.__keys = keys
        self.__editable = False
        self.uri = uri
        self.ctx = ctx

        #set the datatabase
        self.database = database_name
        if record_type is not None:
            self.record_type = record_type

        self.get_selection().set_mode(gtk.SELECTION_MULTIPLE)

    @property
    def keys(self):
        """ set keys - A list of strings to act as keys for the
        TreeView.

        Note that the CouchGrid uses keys for keys, and visa versa.
        If a record does not contain a value for a specified value
        the CouchGrid will simply display an empty string for the
        value. If the widget is set to editable, the user will be able
        to add values to the database.

        Setting this property will cause the widget to reload.

        """

        return self.__keys

    @keys.setter
    def keys(self, keys):
        self.__keys = keys
        if self.record_type != None:
            self.__populate_treeview()

    @property
    def editable(self):
        """editable - bool value, True to make editable
        If set to True, changes are immediately
        persisted to the database.

        """
        return self.__editable

    @editable.setter
    def editable(self, editable):
        self.__editable = editable

        #refresh the treeview if possible
        if self.record_type != None:
            self.__populate_treeview()

    @property
    def database(self):
        """database - gets an instance to the CouchDB.
        Set to a string to change the database.

        """
        return self.__db

    @database.setter
    def database(self, db_name):
        if self.uri:
            self.__db = CouchDatabase(db_name, create=True, uri=self.uri, ctx=self.ctx)
        else:
            self.__db = CouchDatabase(db_name, create=True, ctx=self.ctx)
        if self.record_type != None:
            self.__populate_treeview()(self.record_type)

    @property
    def record_type(self):
        """record_type - a string specifying the record type of
        the documents to retrieve from the CouchDB.

        Will cause the TreeView to refresh when set.
        """
        return self.__record_type

    @record_type.setter
    def record_type(self, record_type):

        #store the record type string
        self.__record_type = record_type
        self.__populate_treeview()

    def __populate_treeview(self):
        #if the database is not set up, just return
        if self.__db == None:
            return

        #column titles are automatically created from keys
        #the cases with titles are complex and hacky but make
        #it easy for the consumer of the api
        #there are the following cases:
        #1. keys have not been set and cannot be inferred
        #2. keys have been previously set and will be use for titles
        #3. titles should be inferred from the keys in the results


        #retrieve the docs for the record_type, if any
        results = self.__db.get_records(
            record_type=self.__record_type,create_view=True)

        #test for keys case 1 and raise an exception
        if len(results) < 1 and self.keys == None:
            raise RuntimeError("""Cannot infer columns for CouchGrid.
        Ensure that records exist, or define columns using
        CouchGrid.keys property.
        """)

        #test for case 2, and set up the model
        if self.keys is not None:
            self.__reset_model()

        #if keys are already assigned, set up titles and columns
        #for keys case 2 with or without results
        if self.keys != None:
            self.__make_headings()

        first_row = True
        for r in results:
            #handle headings case 3
            #if keys are not set up, infer titles them from
            if self.keys == None and first_row:
                self.keys = []
                for k in r.value.keys():
                    if not k.startswith("_") and k != "record_type":
                        self.keys.append(k)
                #now set up the columns and headers
                self.__make_headings()

                #in keys case 3 the model has not been set yet
                #so do that now
                self.__reset_model()

            first_row = False

            #lists have to match the list_store columns in length
            #so we have to make rows as long as the headerings
            #note that the last value is reserved for the doc_id
            col_count = len(self.keys) + 1
            row = ["" for i in xrange(col_count)]

            #replace the values in the row with the values in the
            #retrieved rows. If the the key is not in the doc,
            #just continue with an empty string
            for i, h in enumerate(self.keys):
                try:
                    row[i] = r.value[h]
                except:
                    pass

            #set the last value as the document_id, and append
            row[-1] = r.value["_id"]
            self.list_store.append(row)

        #apply the model tot he Treeview
        self.set_model(self.list_store)

    def append_row(self, new_row, auto_save = True):
        """append_row: add a row to the TreeView.

        arguments:
        new_row - a list of string values to add to the TreeView.
        Empty strings will appear as empty cells. It is not required
        to specify all values to include in the new row, as
        append_row will copy all the values starting at position 0
        and will fill in the remaining values with empty strings.

        auto_save - if True will immediately commit the row
        to the database. Otherwise, the row won't be saved until
        a cell is edited. Typically should be False if adding an
        empty row. An empty list will not be autosaved, even if
        auto_save is set to True.

        Defaults to True.

        """

        #TODO: add gaurd conditions
        #throw if no database is set

        #lists have to match the list_store columns in length
        #note that the last value is reserved for the doc_id
        col_count = len(self.keys) + 1
        row = ["" for i in xrange(col_count)]

        #create a document for autosaving
        doc = {"record_type":self.record_type}

        #copy the values into the row
        #add to the document in case it needs to be auto saved
        for i, v in enumerate(new_row):
            row[i] = new_row[i]
            doc[self.keys[i]] = new_row[i]

        rec = Record(doc)

        #auto save non-empty rows
        if auto_save and len(new_row) > 0:
            doc_id = self.__db.put_record(rec)
            row[len(self.keys)] = doc_id #store the id

        #add it to the list store
        self.list_store.append(row)

    @property
    def selected_rows(self):
        """ selected_rows - returns a list of dictionaries
        for each row selected. Note that the values are not
        necessarily the complete dictionary for the records, but rather
        are determined by the keys used for the CouchGrid.

        To get the complete record, use selected_records, or for the record
        ids, use selected_record_ids

        This property is read only.
        """

        #get the selected rows in the ListStore
        selection = self.get_selection()
        model, model_rows = selection.get_selected_rows()

        rows = [] #list of rows to return
        for mr in model_rows:
            row = {} #a row to be added to the list of rows
            iter = model.get_iter(mr)

            #get the value for each heading and add it to the row
            for i, h in enumerate(self.keys):
                row[h] = model.get_value(iter,i)

            #add the row to the list
            rows.append(row)
        return rows

    @property
    def selected_record_ids(self):
        """ selected_record_ids - a list of document ids that are
        selected in the CouchGrid. Throws an IndexError if
        a specified id is not found in the list when setting
        this property.

        This property is read/write

        """

        #get the selected rows in the ListStore
        selection = self.get_selection()
        model, model_rows = selection.get_selected_rows()

        ids = [] # a list of ids to return
        for mr in model_rows:
            iter = model.get_iter(mr)

            #add the id to the list
            id_index = len(self.keys) #id is always last column
            ids.append(model.get_value(iter,id_index))
        return ids


    @selected_record_ids.setter
    def selected_record_ids(self, indexes):
        rows = [] #a list of rows to select
        for id in indexes:
            id_found = False #track if the id was found

            for i,r in enumerate(self.list_store):
                id_index = len(self.keys) #id is always last column
                if r[id_index] == id: #id matched in ListModel
                    id_found = True #id was good
                    if r not in rows: #don't have duplicates to select
                        rows.append(i)
            if not id_found: #stop if a requested id was not in the list
                raise IndexError("id %s not found" %id)

        #select the requested ids
        selection = self.get_selection()
        selection.unselect_all()
        for r in rows:
            selection.select_path(r)


    def __reset_model(self):
        """ __reset_model - internal funciton, do not call directly.
        This function is typically called when properties are set

        """

        #remove the current columns from the TreeView
        cols = self.get_columns()
        for c in cols:
            self.remove_column(c)

        #make a new ListStore
        col_count = len(self.keys) + 1
        self.list_store = gtk.ListStore(
            *[gobject.TYPE_STRING for i in xrange(col_count)])

    def __make_headings(self):
        """ __make_headings - internal funciton do not call direclty
        Sets up the keys and column titles for the TreeView.

        """

        for i, h in enumerate(self.keys):
            #set up cell renderer
            rend = gtk.CellRendererText()
            rend.mode = gtk.CELL_RENDERER_MODE_EDITABLE
            rend.set_property("editable", self.editable)
            #connect to save function
            rend.connect("edited", self.__edited,i)

            #create a column and add to the TreeView
            col = gtk.TreeViewColumn(h, rend, text=i)
            self.append_column(col)

    def __edited(self, cellrenderertext, path, new_text, col):
        """ __edited - internal signal handler.
        Updates the database if a cell in the Treeview
        has been edited.

        """

        #get an iterator that points to the edited row
        iter = self.list_store.get_iter(path)

        #update the ListStore with the new text
        self.list_store.set_value(iter, col, new_text)

        #get the key/heading for the edited cell
        #as well as the id
        key = self.keys[col]
        id = self.list_store.get_value(iter, len(self.keys))

        if id == "": #the row has not been stored
            #create a document
            doc = {"record_type":self.record_type}

            for i, h in enumerate(self.keys):
                #copy the all the values into the doc
                doc[h] = self.list_store.get_value(iter,i)

            rec = Record(doc)
            #save the document and store the id
            doc_id = self.__db.put_record(rec)
            self.list_store.set_value(iter, len(self.keys), doc_id)

        else: #it has been saved
            self.__db.update_fields(id,{key:new_text})

def __show_selected(widget, widgets):
    """Test function for selection properties of CouchGrid"""
    tv, cw = widgets
    disp = "Rows:\n"
    for r in cw.selected_rows:
        disp += str(r) + "\n"

    disp += "\n\n_Ids:\n"
    for r in cw.selected_record_ids:
        disp += str(r) + "\n"

    tv.get_buffer().set_text(disp)

def __select_ids(widget, widgets):
    entry, cw, lbl = widgets
    try:
        cw.selected_record_ids = entry.get_text().split(",")
    except Exception, inst:
        lbl.set_text(str(inst))

if __name__ == "__main__":
    """creates a test CouchGrid if called directly"""

    #create and show a test window
    win = gtk.Window(gtk.WINDOW_TOPLEVEL)
    win.set_title("CouchGrid Test Window")
    win.connect("destroy",gtk.main_quit)
    win.show()

    #create a top level container
    vbox = gtk.VBox(False, False)
    vbox.show()
    win.add(vbox)

    #create a test widget with test database values
    cw = CouchGrid("couch_widget_test", record_type="test_record_type",
                     keys=["Key1", "Key2", "Key3", "Key4"])

    #allow editing
    cw.editable = True

    #create a row with all four columns set
    cw.append_row(["val1","val2","val3","val4"])

    #create a row with only the second column set
    cw.append_row(["","val2"])

    #create an empty row (which will not be saved until
    #the user edits it)
    cw.append_row([])

    #show the control, add it to the window, and run the main loop
    cw.show()
    vbox.pack_start(cw, False, True)

    #create a test display area
    hbox = gtk.HBox(False, 5)
    hbox.show()
    tv = gtk.TextView()
    tv.show()
    cw.connect("cursor-changed",__show_selected, (tv,cw))

    #create ui for testing selection
    id_vbox = gtk.VBox(False, 5)
    id_vbox.show()

    fb_lbl = gtk.Label("paste ids into the edit box to select them")
    fb_lbl.show()

    entry = gtk.Entry()
    entry.show()

    btn = gtk.Button("select ids")
    btn.show()
    btn.connect("clicked", __select_ids, (entry,cw, fb_lbl))

    id_vbox.pack_start(fb_lbl, False, False)
    id_vbox.pack_start(entry, False, False)
    id_vbox.pack_end(btn, False, False)

    #pack up the window
    hbox.pack_start(tv, False, False)
    vbox.pack_end(hbox, False, False)
    hbox.pack_end(id_vbox, False, False)

    #run the test app
    gtk.main()

