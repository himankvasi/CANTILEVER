/*
 * Inventory Management System
 * Cantilever C++ Internship - Project 01
 *
 * Console application to manage products, track stock levels and
 * generate reports. Data is persisted using File I/O and the
 * Standard Template Library (map, vector, algorithm) is used
 * for in-memory management.
 */

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const string DATA_FILE = "inventory.txt";
const string REPORT_FILE = "inventory_report.txt";

// ----------------------------------------------------------------------------
// Data model
// ----------------------------------------------------------------------------
struct Product {
    int id = 0;
    string name;
    string category;
    int quantity = 0;
    double price = 0.0;
    int reorderLevel = 0;
};

// ----------------------------------------------------------------------------
// Input helpers (robust against bad input and end-of-input)
// ----------------------------------------------------------------------------
string readLine(const string &prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) {
        cout << "\nInput closed. Exiting.\n";
        exit(0);
    }
    // '|' is our file delimiter, so it is not allowed inside text fields
    replace(s.begin(), s.end(), '|', '/');
    return s;
}

int readInt(const string &prompt, int minVal = 0,
            int maxVal = numeric_limits<int>::max()) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t pos;
            int v = stoi(s, &pos);
            if (pos == s.size() && v >= minVal && v <= maxVal) return v;
        } catch (...) {
        }
        if (maxVal == numeric_limits<int>::max())
            cout << "  Invalid number. Enter a whole number >= " << minVal << ".\n";
        else
            cout << "  Invalid number. Enter a value between " << minVal
                 << " and " << maxVal << ".\n";
    }
}

double readDouble(const string &prompt, double minVal = 0.0) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t pos;
            double v = stod(s, &pos);
            if (pos == s.size() && v >= minVal) return v;
        } catch (...) {
        }
        cout << "  Invalid amount. Enter a number >= " << minVal << ".\n";
    }
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return tolower(c); });
    return s;
}

string currentTime() {
    time_t now = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buf;
}

// ----------------------------------------------------------------------------
// Inventory class
// ----------------------------------------------------------------------------
class Inventory {
private:
    map<int, Product> items;  // key = product id (kept sorted by STL map)
    int nextId = 1001;

    void printHeader() const {
        cout << "\n" << left << setw(7) << "ID" << setw(22) << "Name"
             << setw(15) << "Category" << right << setw(8) << "Qty"
             << setw(12) << "Price" << setw(9) << "Reorder" << "\n";
        cout << string(73, '-') << "\n";
    }

    void printRow(const Product &p) const {
        cout << left << setw(7) << p.id << setw(22) << p.name.substr(0, 20)
             << setw(15) << p.category.substr(0, 13) << right << setw(8)
             << p.quantity << setw(12) << fixed << setprecision(2) << p.price
             << setw(9) << p.reorderLevel;
        if (p.quantity <= p.reorderLevel) cout << "  <-- LOW";
        cout << "\n";
    }

public:
    // ---------------- File I/O ----------------
    void load() {
        ifstream in(DATA_FILE);
        if (!in) return;  // first run - no file yet
        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string f[6];
            bool ok = true;
            for (int i = 0; i < 6; i++)
                if (!getline(ss, f[i], '|')) ok = false;
            if (!ok) continue;
            try {
                Product p;
                p.id = stoi(f[0]);
                p.name = f[1];
                p.category = f[2];
                p.quantity = stoi(f[3]);
                p.price = stod(f[4]);
                p.reorderLevel = stoi(f[5]);
                items[p.id] = p;
                nextId = max(nextId, p.id + 1);
            } catch (...) {
            }
        }
    }

    void save() const {
        ofstream out(DATA_FILE);
        for (const auto &kv : items) {
            const Product &p = kv.second;
            out << p.id << '|' << p.name << '|' << p.category << '|'
                << p.quantity << '|' << p.price << '|' << p.reorderLevel
                << '\n';
        }
    }

    // ---------------- Features ----------------
    void addProduct() {
        cout << "\n=== ADD NEW PRODUCT ===\n";
        Product p;
        p.id = nextId++;
        do {
            p.name = readLine("Product name        : ");
        } while (p.name.empty());
        p.category = readLine("Category            : ");
        if (p.category.empty()) p.category = "General";
        p.quantity = readInt("Initial quantity    : ", 0);
        p.price = readDouble("Unit price          : ", 0.0);
        p.reorderLevel = readInt("Reorder level       : ", 0);
        items[p.id] = p;
        save();
        cout << "Product added successfully with ID " << p.id << ".\n";
    }

    void viewAll() const {
        cout << "\n=== ALL PRODUCTS ===";
        if (items.empty()) {
            cout << "\nNo products in inventory.\n";
            return;
        }
        printHeader();
        for (const auto &kv : items) printRow(kv.second);
        cout << string(73, '-') << "\nTotal products: " << items.size() << "\n";
    }

    void search() const {
        cout << "\n=== SEARCH PRODUCT ===\n";
        string key = toLower(readLine("Enter ID, name or category: "));
        vector<const Product *> found;
        for (const auto &kv : items) {
            const Product &p = kv.second;
            if (to_string(p.id) == key ||
                toLower(p.name).find(key) != string::npos ||
                toLower(p.category).find(key) != string::npos)
                found.push_back(&p);
        }
        if (found.empty()) {
            cout << "No matching products found.\n";
            return;
        }
        printHeader();
        for (auto p : found) printRow(*p);
        cout << found.size() << " product(s) found.\n";
    }

    void updateProduct() {
        cout << "\n=== UPDATE PRODUCT ===\n";
        int id = readInt("Enter product ID: ", 1);
        auto it = items.find(id);
        if (it == items.end()) {
            cout << "Product not found.\n";
            return;
        }
        Product &p = it->second;
        cout << "Leave a field blank to keep the current value.\n";
        string s = readLine("Name [" + p.name + "]: ");
        if (!s.empty()) p.name = s;
        s = readLine("Category [" + p.category + "]: ");
        if (!s.empty()) p.category = s;
        s = readLine("Price [" + to_string(p.price).substr(0, to_string(p.price).find('.') + 3) + "]: ");
        if (!s.empty()) {
            try { p.price = max(0.0, stod(s)); } catch (...) { cout << "  Price unchanged (invalid).\n"; }
        }
        s = readLine("Reorder level [" + to_string(p.reorderLevel) + "]: ");
        if (!s.empty()) {
            try { p.reorderLevel = max(0, stoi(s)); } catch (...) { cout << "  Reorder level unchanged (invalid).\n"; }
        }
        save();
        cout << "Product updated.\n";
    }

    void deleteProduct() {
        cout << "\n=== DELETE PRODUCT ===\n";
        int id = readInt("Enter product ID: ", 1);
        auto it = items.find(id);
        if (it == items.end()) {
            cout << "Product not found.\n";
            return;
        }
        string c = readLine("Delete '" + it->second.name + "'? (y/n): ");
        if (toLower(c) == "y") {
            items.erase(it);
            save();
            cout << "Product deleted.\n";
        } else {
            cout << "Cancelled.\n";
        }
    }

    void stockIn() {
        cout << "\n=== STOCK IN (RESTOCK) ===\n";
        int id = readInt("Enter product ID: ", 1);
        auto it = items.find(id);
        if (it == items.end()) {
            cout << "Product not found.\n";
            return;
        }
        int q = readInt("Quantity received: ", 1);
        it->second.quantity += q;
        save();
        cout << "Stock updated. " << it->second.name << " now has "
             << it->second.quantity << " units.\n";
    }

    void stockOut() {
        cout << "\n=== STOCK OUT (SELL / ISSUE) ===\n";
        int id = readInt("Enter product ID: ", 1);
        auto it = items.find(id);
        if (it == items.end()) {
            cout << "Product not found.\n";
            return;
        }
        Product &p = it->second;
        cout << "Available: " << p.quantity << " units\n";
        int q = readInt("Quantity to issue: ", 1);
        if (q > p.quantity) {
            cout << "Insufficient stock! Transaction rejected.\n";
            return;
        }
        p.quantity -= q;
        save();
        cout << "Issued " << q << " x " << p.name << "  (bill amount: "
             << fixed << setprecision(2) << q * p.price << ")\n";
        if (p.quantity <= p.reorderLevel)
            cout << "WARNING: '" << p.name << "' is at or below its reorder level ("
                 << p.reorderLevel << ").\n";
    }

    // ---------------- Reports ----------------
    void lowStockReport() const {
        cout << "\n=== LOW STOCK REPORT ===";
        vector<const Product *> low;
        for (const auto &kv : items)
            if (kv.second.quantity <= kv.second.reorderLevel)
                low.push_back(&kv.second);
        if (low.empty()) {
            cout << "\nAll products are sufficiently stocked.\n";
            return;
        }
        sort(low.begin(), low.end(), [](const Product *a, const Product *b) {
            return a->quantity < b->quantity;
        });
        printHeader();
        for (auto p : low) printRow(*p);
        cout << low.size() << " product(s) need restocking.\n";
    }

    void valuationReport() const {
        cout << "\n=== INVENTORY VALUATION REPORT ===\n";
        if (items.empty()) {
            cout << "No products in inventory.\n";
            return;
        }
        map<string, pair<int, double>> byCat;  // category -> (units, value)
        double total = 0;
        int units = 0;
        for (const auto &kv : items) {
            const Product &p = kv.second;
            double v = p.quantity * p.price;
            byCat[p.category].first += p.quantity;
            byCat[p.category].second += v;
            total += v;
            units += p.quantity;
        }
        cout << left << setw(18) << "Category" << right << setw(10) << "Units"
             << setw(16) << "Value" << "\n" << string(44, '-') << "\n";
        for (const auto &kv : byCat)
            cout << left << setw(18) << kv.first << right << setw(10)
                 << kv.second.first << setw(16) << fixed << setprecision(2)
                 << kv.second.second << "\n";
        cout << string(44, '-') << "\n"
             << left << setw(18) << "TOTAL" << right << setw(10) << units
             << setw(16) << total << "\n";
        const Product *top = nullptr;
        for (const auto &kv : items)
            if (!top || kv.second.quantity * kv.second.price > top->quantity * top->price)
                top = &kv.second;
        cout << "Most valuable stock: " << top->name << " ("
             << top->quantity * top->price << ")\n";
    }

    void sortedView() const {
        cout << "\n=== SORTED VIEW ===\n1. By name\n2. By quantity (low to high)\n3. By price (high to low)\n";
        int c = readInt("Choice: ", 1, 3);
        vector<Product> v;
        for (const auto &kv : items) v.push_back(kv.second);
        if (c == 1)
            sort(v.begin(), v.end(), [](const Product &a, const Product &b) {
                return toLower(a.name) < toLower(b.name);
            });
        else if (c == 2)
            sort(v.begin(), v.end(), [](const Product &a, const Product &b) {
                return a.quantity < b.quantity;
            });
        else
            sort(v.begin(), v.end(), [](const Product &a, const Product &b) {
                return a.price > b.price;
            });
        if (v.empty()) {
            cout << "No products in inventory.\n";
            return;
        }
        printHeader();
        for (const auto &p : v) printRow(p);
    }

    void exportReport() const {
        ofstream out(REPORT_FILE);
        if (!out) {
            cout << "Could not create report file.\n";
            return;
        }
        double total = 0;
        out << "INVENTORY REPORT - generated " << currentTime() << "\n";
        out << string(73, '=') << "\n";
        out << left << setw(7) << "ID" << setw(22) << "Name" << setw(15)
            << "Category" << right << setw(8) << "Qty" << setw(12) << "Price"
            << setw(9) << "Value" << "\n";
        for (const auto &kv : items) {
            const Product &p = kv.second;
            out << left << setw(7) << p.id << setw(22) << p.name.substr(0, 20)
                << setw(15) << p.category.substr(0, 13) << right << setw(8)
                << p.quantity << setw(12) << fixed << setprecision(2)
                << p.price << setw(9) << p.quantity * p.price << "\n";
            total += p.quantity * p.price;
        }
        out << string(73, '=') << "\nTotal products : " << items.size()
            << "\nTotal stock value: " << total << "\n";
        cout << "Report exported to '" << REPORT_FILE << "'.\n";
    }
};

// ----------------------------------------------------------------------------
// Menu
// ----------------------------------------------------------------------------
void showMenu() {
    cout << "\n==========================================\n"
         << "      INVENTORY MANAGEMENT SYSTEM\n"
         << "==========================================\n"
         << " 1. Add product           7. Stock out (sell)\n"
         << " 2. View all products     8. Low stock report\n"
         << " 3. Search product        9. Valuation report\n"
         << " 4. Update product       10. Sorted view\n"
         << " 5. Delete product       11. Export report to file\n"
         << " 6. Stock in (restock)    0. Exit\n"
         << "------------------------------------------\n";
}

int main() {
    Inventory inv;
    inv.load();
    while (true) {
        showMenu();
        int choice = readInt("Enter your choice: ", 0, 11);
        switch (choice) {
            case 1: inv.addProduct(); break;
            case 2: inv.viewAll(); break;
            case 3: inv.search(); break;
            case 4: inv.updateProduct(); break;
            case 5: inv.deleteProduct(); break;
            case 6: inv.stockIn(); break;
            case 7: inv.stockOut(); break;
            case 8: inv.lowStockReport(); break;
            case 9: inv.valuationReport(); break;
            case 10: inv.sortedView(); break;
            case 11: inv.exportReport(); break;
            case 0:
                inv.save();
                cout << "Data saved. Goodbye!\n";
                return 0;
        }
    }
}
