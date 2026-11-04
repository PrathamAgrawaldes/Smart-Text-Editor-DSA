
#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <memory>
#include <algorithm>
#include "include/httplib.h"
#include "include/json.hpp"

using namespace std;
using json = nlohmann::json;

// Doubly linked list node for editing history
struct HistoryNode {
    string operation;
    HistoryNode* prev;
    HistoryNode* next;

    HistoryNode(string op) {
        operation = op;
        prev = nullptr;
        next = nullptr;
    }
};

class Editor {
private:
    string text = "";

    stack<string> undoStack;
    stack<string> redoStack;

    HistoryNode* head = nullptr;
    HistoryNode* tail = nullptr;

    vector<string> history;

    void addHistory(string operation) {
        HistoryNode* node = new HistoryNode(operation);

        if (head == nullptr) {
            head = tail = node;
        } else {
            tail->next = node;
            node->prev = tail;
            tail = node;
        }

        history.push_back(operation);
    }

    void saveState() {
        undoStack.push(text);

        while (!redoStack.empty()) {
            redoStack.pop();
        }
    }

public:
    ~Editor() {
        HistoryNode* current = head;

        while (current != nullptr) {
            HistoryNode* next = current->next;
            delete current;
            current = next;
        }
    }

    string getText() {
        return text;
    }

    vector<string> getHistory() {
        return history;
    }

    bool replaceText(int start, int end, string replacement) {
        if (start < 0 || end < start ||
            end > static_cast<int>(text.size())) {
            return false;
        }

        if (text.substr(start, end - start) == replacement) {
            return true;
        }

        saveState();

        text.replace(start, end - start, replacement);
        addHistory("REPLACE");

        return true;
    }

    bool setText(string newText, string operation) {
        if (newText == text) {
            return true;
        }

        saveState();
        text = newText;
        addHistory(operation);

        return true;
    }

    bool undo() {
        if (undoStack.empty()) {
            return false;
        }

        redoStack.push(text);
        text = undoStack.top();
        undoStack.pop();

        addHistory("UNDO");
        return true;
    }

    bool redo() {
        if (redoStack.empty()) {
            return false;
        }

        undoStack.push(text);
        text = redoStack.top();
        redoStack.pop();

        addHistory("REDO");
        return true;
    }

    json getState() {
        return {
            {"text", text},
            {"history", history},
            {"canUndo", !undoStack.empty()},
            {"canRedo", !redoStack.empty()}
        };
    }
};

int main() {
    httplib::Server server;
    Editor editor;

    // Allow requests from the frontend
    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    // Handle browser CORS preflight requests
    server.Options(R"(/.*)", [](const httplib::Request&,
                                httplib::Response& res) {
        res.status = 204;
    });

    // Get current editor state
    server.Get("/api/state", [&](const httplib::Request&,
                                 httplib::Response& res) {
        res.set_content(editor.getState().dump(), "application/json");
    });

    // Apply a text change
    server.Post("/api/edit", [&](const httplib::Request& req,
                                 httplib::Response& res) {
        try {
            json data = json::parse(req.body);

            if (!data.contains("text") || !data["text"].is_string()) {
                res.status = 400;
                res.set_content(
                    R"({"error":"A text string is required."})",
                    "application/json"
                );
                return;
            }

            string operation = "EDIT";

            if (data.contains("operation") &&
                data["operation"].is_string()) {
                operation = data["operation"].get<string>();
            }

            // Only accept known operation labels
            if (operation != "TYPE" &&
                operation != "PASTE" &&
                operation != "DELETE" &&
                operation != "EDIT") {
                operation = "EDIT";
            }

            editor.setText(data["text"].get<string>(), operation);

            res.set_content(editor.getState().dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(
                R"({"error":"Invalid JSON request."})",
                "application/json"
            );
        }
    });

    // Replace a selected range of text
    server.Post("/api/replace", [&](const httplib::Request& req,
                                    httplib::Response& res) {
        try {
            json data = json::parse(req.body);

            if (!data.contains("start") ||
                !data.contains("end") ||
                !data.contains("text") ||
                !data["start"].is_number_integer() ||
                !data["end"].is_number_integer() ||
                !data["text"].is_string()) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid replacement data."})",
                    "application/json"
                );
                return;
            }

            int start = data["start"].get<int>();
            int end = data["end"].get<int>();
            string replacement = data["text"].get<string>();

            if (!editor.replaceText(start, end, replacement)) {
                res.status = 400;
                res.set_content(
                    R"({"error":"Invalid text selection."})",
                    "application/json"
                );
                return;
            }

            res.set_content(editor.getState().dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(
                R"({"error":"Invalid JSON request."})",
                "application/json"
            );
        }
    });

    // Undo
    server.Post("/api/undo", [&](const httplib::Request&,
                                 httplib::Response& res) {
        bool success = editor.undo();

        json result = editor.getState();
        result["success"] = success;

        if (!success) {
            result["message"] = "Nothing to undo!";
        }

        res.set_content(result.dump(), "application/json");
    });

    // Redo
    server.Post("/api/redo", [&](const httplib::Request&,
                                 httplib::Response& res) {
        bool success = editor.redo();

        json result = editor.getState();
        result["success"] = success;

        if (!success) {
            result["message"] = "Nothing to redo!";
        }

        res.set_content(result.dump(), "application/json");
    });

    cout << "Smart Text Editor server running at "
         << "http://localhost:8080\n";

    cout << "Keep this terminal open while using the editor.\n";

    server.listen("127.0.0.1", 8080);

    return 0;
}
