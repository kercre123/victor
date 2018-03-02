#! /usr/bin/env python3

import os
from datetime import datetime
import json

from tkinter import *
from tkinter.scrolledtext import ScrolledText
from tkinter import filedialog

import codelabImport

root = Tk()

rootFrame = Frame(root)
rootFrame.pack()

bottomFrame = Frame(root)
bottomFrame.pack()

log = ScrolledText(bottomFrame, height=6, wrap=WORD, takefocus=0, relief='sunken',bd=1)
log.pack()

def log_text(text):
    log.insert(END, text + '\n')
    log.see(END)

def TextEntry(name, default='', **options):
    frame = Frame(rootFrame)
    Label(frame, text="{}:".format(name), width=20, anchor=E).pack(side=LEFT)
    field = Entry(frame, **options)
    field.insert(0,default)
    field.pack(side=LEFT)
    frame.pack(fill=X)
    return field

def MultilineTextEntry(name, default='', **options):
    frame = Frame(rootFrame)
    Label(frame, text="{}:".format(name), width=20, anchor=E).pack(side=LEFT)
    field = ScrolledText(frame, bd=1, relief='sunken', **options)
    field.pack(side=LEFT)
    frame.pack(fill=X)
    return field
    
def FileEntry(name, filetypes):
    frame = Frame(rootFrame)
    Label(frame, text="{}:".format(name), width=20, anchor=E).pack(side=LEFT)
    field = Entry(frame, width=50)
    field.pack(side=LEFT)
    def SelectFile():
        field.delete(0,END)
        field.insert(0, filedialog.askopenfilename(filetypes = filetypes))
    button = Button(frame, text='Open...', command=SelectFile)
    button.pack(side=LEFT)
    frame.pack(fill=X)
    return field
    
def ListBoxEntry(name, choices):
    frame = Frame(rootFrame)
    frame.pack(fill=X)
    Label(frame, text="{}:".format(name), width=20, anchor=E).pack(side=LEFT)
    var = StringVar(frame)
    var.set(choices[0])
    field = OptionMenu(frame, var, *choices)
    field.pack(side=LEFT)
    return var

def DateEntry(name):
    frame = Frame(rootFrame)
    frame.pack(fill=X)
    Label(frame, text="{}:".format(name), width=20, anchor=E).pack(side=LEFT)
    now = datetime.now()
    year = Entry(frame, width=4)
    year.insert(0, str(now.year))
    year.pack(side=LEFT)
    month = Entry(frame, width=2)
    month.insert(0, str(now.month))
    month.pack(side=LEFT)
    day = Entry(frame, width=2)
    day.insert(0, str(now.day))
    day.pack(side=LEFT)
    return (year, month, day)

# Setup UI
nameField = TextEntry('Project Name', width=25)
codelabFile = FileEntry('.codelab File', (("codelab files", "*.codelab"),))
description = MultilineTextEntry('Description', width=50, height=6)
instructions = MultilineTextEntry('Instructions', width=50, height=6)
bgColor = TextEntry('BG Color', default='#ffe700')
textColor = TextEntry('Text Color', default='#696b68')
framingCardPath = FileEntry('Framing Card Image', (("Image files", "*.jpg"),))
activeIconPath = FileEntry('Active Icon Image', (("Image files", "*.jpg"),))
idleIconPath = FileEntry('Idle Icon Image', (("Image files", "*.jpg"),))
whatsNewIconPath = FileEntry('Whats New Icon Image', (("Image files", "*.png"),))
whatsNewTint = ListBoxEntry('Whats New Tint', ['yellow', 'purple', 'red', 'green'])
startYear, startMonth, startDay = DateEntry('Start Date')
endYear, endMonth, endDay = DateEntry('End Date')

def testEmpty(name, value):
    if len(value) == 0:
        log_text('Error: {} field is empty.'.format(name))
        return 1
    return 0

def Import():
    errors = 0
    errors += testEmpty('Project Name', nameField.get())
    errors += testEmpty('.codelab File', codelabFile.get())
    errors += testEmpty('Description', codelabFile.get())
    errors += testEmpty('Instructions', codelabFile.get())
    errors += testEmpty('BG Color', codelabFile.get())
    errors += testEmpty('Text Color', codelabFile.get())
    errors += testEmpty('Framing Card Image', codelabFile.get())
    errors += testEmpty('Active Icon Image', codelabFile.get())
    errors += testEmpty('Idle Icon Image', codelabFile.get())
    errors += testEmpty('Whats New Icon Image', codelabFile.get())

    if(errors > 0):
        log_text('Import aborted, please fix errors above and try again.')
        return
    
    tints = {
        'yellow':([212,172,3], [255,220,26]),
        'purple':([130,31,183], [150,73,190]),
        'red':([195,50,50], [253,78,93]),
        'green':([93,197,21], [157,227,53])
    }
    
    tint = tints[whatsNewTint.get()]
    
    startDate = datetime(int(startYear.get()), int(startMonth.get()), int(startDay.get()))
    endDate = datetime(int(endYear.get()), int(endMonth.get()), int(endDay.get()))
    
    codelabImport.log_text = log_text # Route codelabImport output to GUI
    codelabImport.importCodelabFile(nameField.get().rstrip(),
                                    codelabFile.get(),
                                    description.get(1.0, END).rstrip(),
                                    instructions.get(1.0, END).rstrip(),
                                    bgColor.get(),
                                    textColor.get(),
                                    framingCardPath.get(),
                                    activeIconPath.get(),
                                    idleIconPath.get(),
                                    tint[0], tint[1],
                                    whatsNewIconPath.get(),
                                    startDate, endDate)

Button(rootFrame, text='Import', command=Import).pack()

log_text('Codelab Importer\nFill in all fields, then click the Import button.\nErrors will be reported in this box.')

root.lift()
root.attributes('-topmost',True)
root.after_idle(root.attributes,'-topmost',False)

root.mainloop()
