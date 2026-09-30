/*
 * Simple Social Network
 * Cantilever C++ Internship - Project 02
 *
 * Console application where users can create profiles, add friends and
 * post messages. Data is persisted with File I/O (users.txt, posts.txt)
 * and managed in memory with the Standard Template Library
 * (map, set, vector, algorithm).
 */

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const string USERS_FILE = "users.txt";
const string POSTS_FILE = "posts.txt";

// ----------------------------------------------------------------------------
// Data model
// ----------------------------------------------------------------------------
struct User {
    string username;
    string password;
    string fullName;
    string bio;
    set<string> friends;  // usernames
};

struct Post {
    int id;
    string author;
    time_t timestamp;
    string text;
};

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------
string readLine(const string &prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) {
        cout << "\nInput closed. Exiting.\n";
        exit(0);
    }
    replace(s.begin(), s.end(), '|', '/');
    return s;
}

int readInt(const string &prompt, int lo, int hi) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t pos;
            int v = stoi(s, &pos);
            if (pos == s.size() && v >= lo && v <= hi) return v;
        } catch (...) {
        }
        cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return tolower(c); });
    return s;
}

string formatTime(time_t t) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
    return buf;
}

vector<string> split(const string &s, char d) {
    vector<string> out;
    string tok;
    stringstream ss(s);
    while (getline(ss, tok, d)) out.push_back(tok);
    return out;
}

// ----------------------------------------------------------------------------
// SocialNetwork class
// ----------------------------------------------------------------------------
class SocialNetwork {
private:
    map<string, User> users;  // username -> User
    vector<Post> posts;
    int nextPostId = 1;
    string current;  // logged-in username ("" if nobody)

    // ---- File I/O ----
    // users.txt line: username|password|fullName|bio|friend1,friend2,...
    void saveUsers() const {
        ofstream out(USERS_FILE);
        for (const auto &kv : users) {
            const User &u = kv.second;
            out << u.username << '|' << u.password << '|' << u.fullName << '|'
                << u.bio << '|';
            bool first = true;
            for (const auto &f : u.friends) {
                if (!first) out << ',';
                out << f;
                first = false;
            }
            out << '\n';
        }
    }

    // posts.txt line: id|author|timestamp|text
    void savePosts() const {
        ofstream out(POSTS_FILE);
        for (const auto &p : posts)
            out << p.id << '|' << p.author << '|' << p.timestamp << '|'
                << p.text << '\n';
    }

    void printPost(const Post &p) const {
        auto it = users.find(p.author);
        string name = it != users.end() ? it->second.fullName : p.author;
        cout << "  [" << formatTime(p.timestamp) << "] " << name << " (@"
             << p.author << ")\n    \"" << p.text << "\"\n";
    }

    bool loggedIn() const { return !current.empty(); }

public:
    void load() {
        ifstream in(USERS_FILE);
        string line;
        while (getline(in, line)) {
            auto f = split(line, '|');
            if (f.size() < 4) continue;
            User u;
            u.username = f[0];
            u.password = f[1];
            u.fullName = f[2];
            u.bio = f[3];
            if (f.size() >= 5)
                for (auto &fr : split(f[4], ',')) u.friends.insert(fr);
            users[u.username] = u;
        }
        ifstream pin(POSTS_FILE);
        while (getline(pin, line)) {
            auto f = split(line, '|');
            if (f.size() < 4) continue;
            try {
                Post p;
                p.id = stoi(f[0]);
                p.author = f[1];
                p.timestamp = (time_t)stoll(f[2]);
                p.text = f[3];
                posts.push_back(p);
                nextPostId = max(nextPostId, p.id + 1);
            } catch (...) {
            }
        }
    }

    const string &currentUser() const { return current; }
    bool isLoggedIn() const { return loggedIn(); }

    // ---- Account features ----
    void createProfile() {
        cout << "\n=== CREATE PROFILE ===\n";
        string uname;
        while (true) {
            uname = toLower(readLine("Choose a username (no spaces): "));
            if (uname.empty() || uname.find(' ') != string::npos ||
                uname.find(',') != string::npos) {
                cout << "  Username cannot be empty or contain spaces/commas.\n";
            } else if (users.count(uname)) {
                cout << "  That username is already taken.\n";
            } else
                break;
        }
        User u;
        u.username = uname;
        do {
            u.password = readLine("Password                      : ");
        } while (u.password.empty());
        u.fullName = readLine("Full name                     : ");
        if (u.fullName.empty()) u.fullName = uname;
        u.bio = readLine("Short bio                     : ");
        users[uname] = u;
        saveUsers();
        cout << "Profile created! You can now log in as @" << uname << ".\n";
    }

    void login() {
        cout << "\n=== LOGIN ===\n";
        string uname = toLower(readLine("Username: "));
        string pass = readLine("Password: ");
        auto it = users.find(uname);
        if (it == users.end() || it->second.password != pass) {
            cout << "Invalid username or password.\n";
            return;
        }
        current = uname;
        cout << "Welcome back, " << it->second.fullName << "!\n";
    }

    void logout() {
        cout << "Goodbye, @" << current << ".\n";
        current.clear();
    }

    // ---- Profile ----
    void viewProfile(const string &uname) const {
        auto it = users.find(uname);
        if (it == users.end()) {
            cout << "User not found.\n";
            return;
        }
        const User &u = it->second;
        int count = count_if(posts.begin(), posts.end(),
                             [&](const Post &p) { return p.author == uname; });
        cout << "\n+----------------------------------------+\n"
             << "  " << u.fullName << "  (@" << u.username << ")\n"
             << "  Bio     : " << u.bio << "\n"
             << "  Friends : " << u.friends.size() << "    Posts: " << count << "\n"
             << "+----------------------------------------+\n";
        if (uname != current && loggedIn())
            cout << (users.at(current).friends.count(uname)
                         ? "  (You are friends)\n"
                         : "  (Not friends yet)\n");
        vector<const Post *> mine;
        for (const auto &p : posts)
            if (p.author == uname) mine.push_back(&p);
        if (!mine.empty()) {
            cout << "Recent posts:\n";
            int shown = 0;
            for (auto it2 = mine.rbegin(); it2 != mine.rend() && shown < 5; ++it2, ++shown)
                printPost(**it2);
        }
    }

    void editBio() {
        string b = readLine("New bio: ");
        users[current].bio = b;
        saveUsers();
        cout << "Bio updated.\n";
    }

    // ---- Friends ----
    void addFriend() {
        cout << "\n=== ADD FRIEND ===\n";
        string f = toLower(readLine("Enter username to add: "));
        if (f == current) {
            cout << "You cannot add yourself.\n";
            return;
        }
        if (!users.count(f)) {
            cout << "No such user.\n";
            return;
        }
        if (users[current].friends.count(f)) {
            cout << "You are already friends with @" << f << ".\n";
            return;
        }
        users[current].friends.insert(f);
        users[f].friends.insert(current);  // friendship is mutual
        saveUsers();
        cout << "You and @" << f << " are now friends!\n";
    }

    void removeFriend() {
        cout << "\n=== REMOVE FRIEND ===\n";
        string f = toLower(readLine("Enter username to remove: "));
        if (!users[current].friends.count(f)) {
            cout << "@" << f << " is not in your friend list.\n";
            return;
        }
        users[current].friends.erase(f);
        users[f].friends.erase(current);
        saveUsers();
        cout << "Friend removed.\n";
    }

    void listFriends() const {
        cout << "\n=== YOUR FRIENDS ===\n";
        const auto &fr = users.at(current).friends;
        if (fr.empty()) {
            cout << "You have no friends yet. Use 'Add friend' to connect.\n";
            return;
        }
        int i = 1;
        for (const auto &f : fr)
            cout << " " << i++ << ". " << users.at(f).fullName << " (@" << f << ")\n";
    }

    void suggestFriends() const {
        cout << "\n=== FRIEND SUGGESTIONS (friends of friends) ===\n";
        const auto &mine = users.at(current).friends;
        map<string, int> mutual;
        for (const auto &f : mine)
            for (const auto &ff : users.at(f).friends)
                if (ff != current && !mine.count(ff)) mutual[ff]++;
        if (mutual.empty()) {
            cout << "No suggestions right now.\n";
            return;
        }
        vector<pair<string, int>> v(mutual.begin(), mutual.end());
        sort(v.begin(), v.end(), [](auto &a, auto &b) { return a.second > b.second; });
        for (auto &s : v)
            cout << " @" << s.first << " - " << s.second << " mutual friend(s)\n";
    }

    void searchUsers() const {
        cout << "\n=== SEARCH USERS ===\n";
        string key = toLower(readLine("Search by name or username: "));
        int n = 0;
        for (const auto &kv : users) {
            if (toLower(kv.second.fullName).find(key) != string::npos ||
                kv.first.find(key) != string::npos) {
                cout << " " << kv.second.fullName << " (@" << kv.first << ")\n";
                n++;
            }
        }
        if (!n) cout << "No users found.\n";
    }

    void viewOtherProfile() const {
        string u = toLower(readLine("Enter username: "));
        viewProfile(u);
    }

    // ---- Posts ----
    void createPost() {
        cout << "\n=== NEW POST ===\n";
        string text = readLine("What's on your mind? ");
        if (text.empty()) {
            cout << "Post cannot be empty.\n";
            return;
        }
        if (text.size() > 280) text.resize(280);
        posts.push_back({nextPostId++, current, time(nullptr), text});
        savePosts();
        cout << "Posted!\n";
    }

    void viewFeed() const {
        cout << "\n=== YOUR FEED ===\n";
        const auto &fr = users.at(current).friends;
        vector<const Post *> feed;
        for (const auto &p : posts)
            if (p.author == current || fr.count(p.author)) feed.push_back(&p);
        if (feed.empty()) {
            cout << "Nothing to show yet. Post something or add friends!\n";
            return;
        }
        sort(feed.begin(), feed.end(), [](const Post *a, const Post *b) {
            return a->timestamp != b->timestamp ? a->timestamp > b->timestamp
                                                : a->id > b->id;
        });
        int shown = 0;
        for (auto p : feed) {
            printPost(*p);
            if (++shown == 10) break;
        }
    }

    void deletePost() {
        cout << "\n=== DELETE POST ===\n";
        vector<const Post *> mine;
        for (const auto &p : posts)
            if (p.author == current) mine.push_back(&p);
        if (mine.empty()) {
            cout << "You have no posts.\n";
            return;
        }
        for (auto p : mine)
            cout << " #" << p->id << "  " << p->text.substr(0, 50) << "\n";
        int id = readInt("Post number to delete (0 to cancel): ", 0, 1000000);
        if (id == 0) return;
        auto it = remove_if(posts.begin(), posts.end(), [&](const Post &p) {
            return p.id == id && p.author == current;
        });
        if (it == posts.end()) {
            cout << "That is not one of your posts.\n";
            return;
        }
        posts.erase(it, posts.end());
        savePosts();
        cout << "Post deleted.\n";
    }
};

// ----------------------------------------------------------------------------
// Menus
// ----------------------------------------------------------------------------
void guestMenu() {
    cout << "\n==========================================\n"
         << "           SIMPLE SOCIAL NETWORK\n"
         << "==========================================\n"
         << " 1. Create profile\n 2. Log in\n 3. Exit\n"
         << "------------------------------------------\n";
}

void userMenu(const string &u) {
    cout << "\n==========================================\n"
         << "  Logged in as @" << u << "\n"
         << "==========================================\n"
         << " 1. View my profile      6. Friend suggestions\n"
         << " 2. Edit bio             7. Search users\n"
         << " 3. Add friend           8. View a profile\n"
         << " 4. Remove friend        9. Post a message\n"
         << " 5. List friends        10. View feed\n"
         << "                        11. Delete a post\n"
         << " 0. Log out\n"
         << "------------------------------------------\n";
}

int main() {
    SocialNetwork net;
    net.load();
    while (true) {
        if (!net.isLoggedIn()) {
            guestMenu();
            int c = readInt("Enter your choice: ", 1, 3);
            if (c == 1) net.createProfile();
            else if (c == 2) net.login();
            else {
                cout << "Thanks for using Simple Social Network. Bye!\n";
                return 0;
            }
        } else {
            userMenu(net.currentUser());
            int c = readInt("Enter your choice: ", 0, 11);
            switch (c) {
                case 1: net.viewProfile(net.currentUser()); break;
                case 2: net.editBio(); break;
                case 3: net.addFriend(); break;
                case 4: net.removeFriend(); break;
                case 5: net.listFriends(); break;
                case 6: net.suggestFriends(); break;
                case 7: net.searchUsers(); break;
                case 8: net.viewOtherProfile(); break;
                case 9: net.createPost(); break;
                case 10: net.viewFeed(); break;
                case 11: net.deletePost(); break;
                case 0: net.logout(); break;
            }
        }
    }
}
