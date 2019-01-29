package dev

import (
	"anki/util/cvars"
	"fmt"
	"html/template"
	"net/http"
	"path"
	"strconv"
)

const baseDir = "/anki/data/assets/cozmo_resources/webserver/cloud/cvars"

func listCVars(w http.ResponseWriter, r *http.Request) {
	vars := toOurVars(cvars.GetAllVars())

	t, err := template.ParseFiles(path.Join(baseDir, "list.html"))
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error parsing template: ", err)
		return
	}

	if err := t.Execute(w, vars); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprint(w, "Error executing template: ", err)
		return
	}
}

type CVar cvars.CVar

func toOurVars(in map[string]cvars.CVar) map[string]CVar {
	ret := make(map[string]CVar)
	for k, v := range in {
		ret[k] = CVar(v)
	}
	return ret
}

func (c CVar) HTML(name string) template.HTML {
	itemID := "id-" + name
	var innerForm string
	var initialVal string
	switch c.VType {
	case cvars.Int:
		innerForm = fmt.Sprintf(
			`%s: <input id="%s" type="number" /> <input type="submit" value="Submit">`,
			name, itemID)
		initialVal = fmt.Sprint(c.Accessor())
	case cvars.String:
		innerForm = fmt.Sprintf(
			`%s: <input id="%s" type="text" value="%s" /> <input type="submit" value="Submit">`,
			name, itemID, c.Accessor())
	case cvars.IntSet:
		currentValue := fmt.Sprint(c.Accessor())
		innerForm = fmt.Sprintf(`%s: <select id="%s">`, name, itemID)
		for _, opt := range c.Options {
			var selected string
			if opt.Title == currentValue {
				selected = ` selected="selected"`
			}
			innerForm += "\n\t" + fmt.Sprintf(`<option value="%s"%s>%s</option>`,
				opt.Value, selected, opt.Title)
		}
		innerForm += "\n\t</select>" + `<input type="submit" value="Submit">`
	}

	return template.HTML(fmt.Sprintf(
		`<div><form onSubmit="return submitRequest('%s', '%s')">%s</form><span id="%s">%s</span></div>`,
		name, itemID, innerForm, itemID+"-resp", initialVal))
}

func cvarHandler(w http.ResponseWriter, r *http.Request) {
	query := r.URL.Query()
	name := query.Get("name")
	value := query.Get("value")

	w.Header().Set("Content-Type", "application/json")

	cv, ok := cvars.GetAllVars()[name]
	if !ok {
		w.Write([]byte(fmt.Sprintf(`{"error": "var %s does not exist"}`, name)))
		return
	}

	resp, err := cv.Set(value)
	if err != nil {
		w.Write([]byte(fmt.Sprintf(`{"error": %s}`, strconv.Quote(err.Error()))))
		return
	}

	w.Write([]byte(fmt.Sprintf(`{"response": %s}`, strconv.Quote(fmt.Sprint(resp)))))
}
