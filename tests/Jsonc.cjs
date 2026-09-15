// Match string literals first; never remove comment-like text within labels.
module.exports = text => JSON.parse(text.replace(/"(?:\\.|[^"\\])*"|\/\/[^\r\n]*|\/\*[\s\S]*?\*\//g,
    token => token.startsWith('"') ? token : token.replace(/[^\r\n]/g, ' ')));
