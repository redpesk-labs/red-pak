#!/bin/bash


export MRGHTML_EXCLUDE="REDPAK.IDX.html"
export TITLE="redpak 2.3"

get_html_list() {
	local seed="$1"
	cat "$seed" |
	grep -o 'href="[^"]*html[^"]*"' |
	sed 's/href="\(.*\.html\).*/\1/' |
	awk '
	  BEGIN { split(ENVIRON["MRGHTML_EXCLUDE"], e, ":"); for(i in e) p[e[i]] = 1}
	  !($1 in p) { print; p[$1]=1 }'
}

HTMLs=$(get_html_list index.html)

main_header() {
cat << EOC
<!doctype html>
<html><head><title>$TITLE</title>
<link rel="stylesheet" type="text/css" href="assets/main.css">
</head><body>
<h1>$TITLE</h1>
EOC
}

main_footer() {
cat << EOC
</body></html>
EOC
}

before() {
	local section="$1"
cat << EOC
<table class="report-container">
<tfoot  class="report-footer">
<tr>
<td class="outer">
<div class="dfoot">
$section
<br>
$TITLE
</div>
</td>
</tr>
</tfoot>
<tbody>
<tr>
<td class="outer">
<div>
EOC
}

after() {
cat << EOC
<div>
</td>
</tr>
</tbody>
</table>
EOC
}

get_title() {
	local file="$1"
	awk '
		/<title/{p=1}
		p{print}
		/<\/title/{exit}' "$file" |
	sed 's:.*<title[^>]*>::' |
	sed 's:</title.*::' |
	sed 's:^[ \t]*::' |
	sed 's:[ \t]*$::' |
	tr -d '\n'
}

get_body() {
	local file="$1"
	awk '
		/<body/{sub(".*<body[^>]*>","");p=1}
		/<\/body/{if(p){sub("</body.*","");print}exit}
		p{print}' "$file"
}

document() {
	local file="$1"
	local title="$(get_title "$file")"
	echo
	echo "<!-- $file -->"
	echo "<a id=\"hsec-$file\"></a>"
	before "$title"
	get_body "$file"
	after
	echo
}

main_header
for x in index.html $HTMLs
do
	document "$x"
done |
sed 's/href="[^#"]*#/href="#/g' |
sed -E 's/href="('"$(echo $HTMLs|tr ' ' '|')"')"/href="#hsec-\1"/g'
main_footer

