
const textArea = document.getElementById("text-area");
const replaceText = document.getElementById("replace-text");

const replaceBtn = document.getElementById("replace-btn");
const undoBtn = document.getElementById("undo-btn");
const redoBtn = document.getElementById("redo-btn");

const statusMessage = document.getElementById("status-message");
const historyList = document.getElementById("history-list");

// Undo and Redo stacks
const undoStack = [];
const redoStack = [];

// Doubly linked list for editing history
class HistoryNode {
    constructor(operation) {
        this.operation = operation;
        this.prev = null;
        this.next = null;
    }
}

let historyHead = null;
let historyTail = null;

// Track the last text state
let lastText = textArea.value;

// Add an operation to the history list
function addHistory(operation) {
    const node = new HistoryNode(operation);

    if (historyHead === null) {
        historyHead = historyTail = node;
    } else {
        historyTail.next = node;
        node.prev = historyTail;
        historyTail = node;
    }

    displayHistory();
}

// Display editing history

function displayHistory() {
    historyList.innerHTML = "";

    if (historyTail === null) {
        historyList.innerHTML = "<li>No editing history yet.</li>";
        return;
    }

    let current = historyTail;
    let count = 1;

    // Display only the latest 10 operations
    while (current !== null && count <= 10) {
        const item = document.createElement("li");
        item.textContent = count + ". " + current.operation;
        historyList.appendChild(item);

        current = current.prev;
        count++;
    }
}

// Show status messages
function showStatus(message) {
    statusMessage.textContent = message;
}

// Record a change to the text
function recordChange(operation) {
    const currentText = textArea.value;

    if (currentText === lastText) {
        return;
    }

    undoStack.push(lastText);
    redoStack.length = 0;

    lastText = currentText;

    addHistory(operation);
    showStatus(operation + " successful.");
}

// Detect normal typing, pasting, and keyboard deletion
textArea.addEventListener("input", function (event) {
    let operation = "EDIT";

    if (event.inputType === "insertText") {
        operation = "TYPE";
    } else if (event.inputType === "insertFromPaste") {
        operation = "PASTE";
    } else if (
        event.inputType === "deleteContentBackward" ||
        event.inputType === "deleteContentForward" ||
        event.inputType.startsWith("delete")
    ) {
        operation = "DELETE";
    }

    recordChange(operation);
});

// Replace selected text
replaceBtn.addEventListener("click", function () {
    const start = textArea.selectionStart;
    const end = textArea.selectionEnd;
    const replacement = replaceText.value;

    if (start === end) {
        showStatus("Select the text you want to replace.");
        return;
    }

    const previousText = textArea.value;

    textArea.setRangeText(
        replacement,
        start,
        end,
        "select"
    );

    // Record this as one editing operation
    recordChange("REPLACE");

    // Restore focus to the editor
    textArea.focus();

    // Avoid an accidental replacement with the same text
    if (textArea.value === previousText) {
        showStatus("The text was not changed.");
    }
});

// Undo the last change
undoBtn.addEventListener("click", function () {
    if (undoStack.length === 0) {
        showStatus("Nothing to undo!");
        return;
    }

    redoStack.push(textArea.value);
    textArea.value = undoStack.pop();

    lastText = textArea.value;

    addHistory("UNDO");
    showStatus("Undo successful.");

    textArea.focus();
});

// Redo the last undone change
redoBtn.addEventListener("click", function () {
    if (redoStack.length === 0) {
        showStatus("Nothing to redo!");
        return;
    }

    undoStack.push(textArea.value);
    textArea.value = redoStack.pop();

    lastText = textArea.value;

    addHistory("REDO");
    showStatus("Redo successful.");

    textArea.focus();
});

// Initial history display
displayHistory();
