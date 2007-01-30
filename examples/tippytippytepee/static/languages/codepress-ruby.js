/*
 * CodePress regular expressions for Ruby syntax highlighting
 */

syntax = [ // Ruby
	/\"(.*?)(\"|<br>|<\/P>)/g,'<s>"$1$2</s>', // strings double quote
	/\'(.*?)(\'|<br>|<\/P>)/g,'<s>\'$1$2</s>', // strings single quote
    /([\$\@\%]+)([\w\.]*)/g,'<a>$1$2</a>', // vars
    /(def\s+)([\w\.]*)/g,'$1<em>$2</em>', // functions
    /\b(alias|and|BEGIN|begin|break|case|class|def|defined|do|else|elsif|END|end|ensure|false|for|if|in|module|next|nil|not|or|redo|rescue|retry|return|self|super|then|true|undef|unless|until|when|while|yield)\b/g,'<b>$1</b>', // reserved words
    /([\(\){}])/g,'<u>$1</u>', // special chars
    /#(.*?)(<br>|<\/P>)/g,'<i>#$1</i>$2', // comments
];

CodePress.initialize();

