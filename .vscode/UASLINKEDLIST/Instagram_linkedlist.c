#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_STRING 100

typedef struct User {
    int id;
    char username[MAX_STRING];
    char email[MAX_STRING];
    char password[MAX_STRING];
    struct User *next;
} User;

typedef struct Post {
    int id;
    int user_id;
    char content[MAX_STRING];
    char media[MAX_STRING];
    int likes;
    struct Post *next;
} Post;

typedef struct Comment {
    int id;
    int post_id;
    int user_id;
    char text[MAX_STRING];
    struct Comment *next;
} Comment;

// Stack for Undo (linked list)
typedef struct UndoNode {
    Post post;
    struct UndoNode *next;
} UndoNode;

// Queue for Notifications (linked list)
typedef struct NotifNode {
    char msg[MAX_STRING * 2];
    struct NotifNode *next;
} NotifNode;

typedef struct {
    User *users;
    Post *posts;
    Comment *comments;
    int user_count;
    int post_count;
    int comment_count;
    int current_user_id;
    int last_post_id;

    UndoNode *undoTop;
    NotifNode *notifFront, *notifRear;
} AppState;

// --- Stack & Queue ---
void pushUndo(AppState *app, Post p) {
    if (p.id == 0) return;
    UndoNode *node = (UndoNode*)malloc(sizeof(UndoNode));
    node->post = p;
    node->next = app->undoTop;
    app->undoTop = node;
}

// Pop the top post from the undo stack
Post popUndo(AppState *app) {
    if (!app->undoTop) {
        Post empty = {0};
        return empty;
    }
    UndoNode *temp = app->undoTop;
    Post p = temp->post;
    app->undoTop = temp->next;
    free(temp);
    return p;
}

// Check if the undo stack is empty
int isUndoEmpty(AppState *app) {
    return app->undoTop == NULL;
}

// Enqueue a notification message
void enqueueNotif(AppState *app, const char *msg) {
    NotifNode *node = (NotifNode*)malloc(sizeof(NotifNode));
    strncpy(node->msg, msg, sizeof(node->msg));
    node->next = NULL;
    if (app->notifRear) {
        app->notifRear->next = node;
    } else {
        app->notifFront = node;
    }
    app->notifRear = node;
}

// Dequeue a notification message
void showNotifications(AppState *app) {
    printf("\n==================[ Notifications ]==================\n");
    NotifNode *curr = app->notifFront;
    int count = 1;
    while (curr) {
        printf("%d. %s\n", count++, curr->msg);
        curr = curr->next;
    }
    if (count == 1) printf(">> Tidak ada notifikasi.\n");
    printf("=====================================================\n");
}

// --- Linked List Insert ---
void insert_user(AppState *app, User u) {
    User *newUser = (User*)malloc(sizeof(User));
    *newUser = u;
    newUser->next = app->users;
    app->users = newUser;
    app->user_count++;
}

// Insert a new post into the linked list
void insert_post(AppState *app, Post p) {
    Post *newPost = (Post*)malloc(sizeof(Post));
    *newPost = p;
    newPost->next = app->posts;
    app->posts = newPost;
    app->post_count++;
}

// Insert a new comment into the linked list
void insert_comment(AppState *app, Comment c) {
    Comment *newComment = (Comment*)malloc(sizeof(Comment));
    *newComment = c;
    newComment->next = app->comments;
    app->comments = newComment;
    app->comment_count++;
}

// --- File I/O (Linked List Version) ---
void load_users(AppState *app) {
    FILE *file = fopen("users.txt", "r");
    if (!file) return;
    User u;
    while (fscanf(file, "%d|%[^|]|%[^|]|%s\n", &u.id, u.username, u.email, u.password) == 4) {
        u.next = NULL;
        insert_user(app, u);
    }
    fclose(file);
}

// Load posts from file and insert into linked list
void load_posts(AppState *app) {
    FILE *file = fopen("posts.txt", "r");
    if (!file) return;
    Post p;
    int max_id = 0;
    while (fscanf(file, "%d|%d|%[^|]|%[^|]|%d\n", &p.id, &p.user_id, p.content, p.media, &p.likes) == 5) {
        p.next = NULL;
        insert_post(app, p);
        if (p.id > max_id) max_id = p.id;
    }
    app->last_post_id = max_id;
    fclose(file);
}

// Load comments from file and insert into linked list
void load_comments(AppState *app) {
    FILE *file = fopen("comments.txt", "r");
    if (!file) return;
    Comment c;
    while (fscanf(file, "%d|%d|%d|%[^]\n", &c.id, &c.post_id, &c.user_id, c.text) == 4) {
        c.next = NULL;
        insert_comment(app, c);
    }
    fclose(file);
}

// Save users, posts, and comments to files
void save_users(AppState *app) {
    FILE *file = fopen("users.txt", "w");
    User *u = app->users;
    while (u) {
        fprintf(file, "%d|%s|%s|%s\n", u->id, u->username, u->email, u->password);
        u = u->next;
    }
    fclose(file);
}

// Save posts and comments to files
void save_posts(AppState *app) {
    FILE *file = fopen("posts.txt", "w");
    Post *p = app->posts;
    while (p) {
        fprintf(file, "%d|%d|%s|%s|%d\n", p->id, p->user_id, p->content, p->media, p->likes);
        p = p->next;
    }
    fclose(file);
}

// Save comments to file
void save_comments(AppState *app) {
    FILE *file = fopen("comments.txt", "w");
    Comment *c = app->comments;
    while (c) {
        fprintf(file, "%d|%d|%d|%s\n", c->id, c->post_id, c->user_id, c->text);
        c = c->next;
    }
    fclose(file);
}

// --- Fitur ---
int signup(AppState *app) {
    User u;
    u.id = app->user_count + 1;
    printf("Username: ");
    scanf(" %[^\n]", u.username);
    printf("Email: ");
    scanf(" %[^\n]", u.email);
    printf("Password: ");
    scanf(" %[^\n]", u.password);
    u.next = NULL;
    insert_user(app, u);
    save_users(app);
    printf("Signup successful. Please login.\n");
    return -1;
}

// Log user activity
void log_activity(const char *msg) {
    FILE *file = fopen("log.txt", "a");
    if (file) {
        fprintf(file, "%s\n", msg);
        fclose(file);
    }
}

// Login
int login(AppState *app) {
    char uname[MAX_STRING], pass[MAX_STRING];
    printf("Username: ");
    scanf(" %[^\n]", uname);
    printf("Password: ");
    scanf(" %[^\n]", pass);
    User *u = app->users;
    while (u) {
        if (strcmp(u->username, uname) == 0 && strcmp(u->password, pass) == 0) {
            printf("Login successful!\n");
            char logmsg[MAX_STRING * 2];
            snprintf(logmsg, sizeof(logmsg), "User %s logged in.", uname);
            log_activity(logmsg);
            return u->id;
        }
        u = u->next;
    }
    printf("Login failed.\n");
    return -1;
}

// Create post
void create_post(AppState *app) {
    Post p;
    p.id = ++app->last_post_id;
    p.user_id = app->current_user_id;
    printf("Media Filename (png, jpg, etc): ");
    scanf(" %[^\n]", p.media);
    printf("Caption: ");
    scanf(" %[^\n]", p.content);
    p.likes = 0;
    p.next = NULL;
    insert_post(app, p);
    save_posts(app);
    printf("Post created.\n");
}

// View posts
void view_posts(AppState *app) {
    printf("\n====================[ Daftar Postingan ]====================\n");
    Post *p = app->posts;
    int ada = 0;
    while (p) {
        if (p->id == 0) { p = p->next; continue; }
        ada = 1;
        printf("\n------------------------------------------------------------\n");
        printf("[%d] User %d: %s (%s) Likes: %d\n", p->id, p->user_id, p->content, p->media, p->likes);
        // Print comments (reverse order)
        int stack_count = 0;
        Comment *stack[1000];
        Comment *c = app->comments;
        while (c) {
            if (c->post_id == p->id) stack[stack_count++] = c;
            c = c->next;
        }
        while (stack_count > 0) {
            Comment *cc = stack[--stack_count];
            printf("  - Comment from User %d: %s\n", cc->user_id, cc->text);
        }
    p = p->next;
    }
    if (!ada) printf("\n>> Belum ada postingan.\n");
    printf("\n============================================================\n");
}
void like_post(AppState *app) {
    int pid;
    printf("Enter post ID to like: ");
    scanf("%d", &pid);
    Post *p = app->posts;
    while (p) {
        if (p->id == pid) {
            p->likes++;
            save_posts(app);
            printf("Post liked!\n");
            char notif[MAX_STRING * 2];
            snprintf(notif, sizeof(notif), "You liked post ID %d", p->id);
            enqueueNotif(app, notif);
            return;
        }
        p = p->next;
    }
    printf("Post not found.\n");
}
void unlike_post(AppState *app) {
    int pid;
    printf("\nEnter post ID to unlike: ");
    scanf("%d", &pid);
    Post *p = app->posts;
    while (p) {
        if (p->id == pid) {
            if (p->user_id != app->current_user_id) {
                printf(">> Anda hanya bisa unlike post milik sendiri!\n");
                return;
            }
            if (p->likes > 0) {
                p->likes--;
                save_posts(app);
                printf("Post unliked!\n");
                char notif[MAX_STRING * 2];
                snprintf(notif, sizeof(notif), "You unliked post ID %d", p->id);
                enqueueNotif(app, notif);
            } else {
                printf("Post already has 0 likes.\n");
            }
            return;
        }
        p = p->next;
    }
    printf("Post not found.\n");
}
void comment_post(AppState *app) {
    int pid;
    Comment c;
    c.id = app->comment_count + 1;
    c.user_id = app->current_user_id;
    printf("Enter post ID to comment: ");
    scanf("%d", &pid);
    c.post_id = pid;
    printf("Comment: ");
    scanf(" %[^\n]", c.text);
    c.next = NULL;
    insert_comment(app, c);
    save_comments(app);
    printf("Comment added.\n");
    char notif[MAX_STRING * 2];
    snprintf(notif, sizeof(notif), "You commented on post ID %d", pid);
    enqueueNotif(app, notif);
}
void delete_post(AppState *app) {
    int pid;
    printf("Enter post ID to delete: ");
    scanf("%d", &pid);
    Post **pp = &app->posts;
    while (*pp) {
        if ((*pp)->id == pid && (*pp)->user_id == app->current_user_id) {
            pushUndo(app, **pp);
            Post *del = *pp;
            *pp = del->next;
            free(del);
            app->post_count--;
            save_posts(app);
            printf("Post deleted. (Undo available)\n");
            char notif[MAX_STRING * 2];
            snprintf(notif, sizeof(notif), "You deleted post ID %d", pid);
            enqueueNotif(app, notif);
            return;
        }
        pp = &(*pp)->next;
    }
    printf("Post not found or you are not the owner.\n");
}
void edit_post(AppState *app) {
    int pid;
    printf("Enter post ID to edit: ");
    scanf("%d", &pid);
    Post *p = app->posts;
    while (p) {
        if (p->id == pid && p->user_id == app->current_user_id) {
            printf("New Media Filename (png, jpg, etc): ");
            scanf(" %[^\n]", p->media);
            printf("New Caption: ");
            scanf(" %[^\n]", p->content);
            save_posts(app);
            printf("Post updated.\n");
            return;
        }
        p = p->next;
    }
    printf("Post not found or unauthorized.\n");
}
void search_by_username(AppState *app) {
    char uname[MAX_STRING];
    printf("Enter username to search posts: ");
    scanf(" %[^\n]", uname);
    int uid = -1;
    User *u = app->users;
    while (u) {
        if (strcmp(u->username, uname) == 0) {
            uid = u->id;
            break;
        }
        u = u->next;
    }
    if (uid == -1) {
        printf("Username not found.\n");
        return;
    }
    Post *p = app->posts;
    while (p) {
        if (p->user_id == uid) {
            printf("[%d] %s (%s) Likes: %d\n", p->id, p->content, p->media, p->likes);
        }
        p = p->next;
    }
}
void view_own_posts(AppState *app) {
    printf("\n=================[ Postingan Anda ]==================\n");
    Post *p = app->posts;
    int ada = 0;
    while (p) {
        if (p->user_id == app->current_user_id) {
            ada = 1;
            printf("[%d] %s (%s) Likes: %d\n", p->id, p->content, p->media, p->likes);
        }
        p = p->next;
    }
    if (!ada) printf(">> Anda belum membuat postingan.\n");
    printf("=====================================================\n");
}

// Function prototype for quick_sort_posts
void quick_sort_posts(Post **arr, int low, int high);

// --- Heap untuk Top Liked Posts ---
void top_liked_posts(AppState *app) {
    printf("\n==================[ Top 3 Posts ]==================\n");
    int n = 0;
    Post *p = app->posts;
    while (p) { n++; p = p->next; }
    if (n == 0) {
        printf("\n>> Tidak ada post.\n");
        printf("===================================================\n");
        return;
    }
    Post **arr = (Post**)malloc(sizeof(Post*) * n);
    p = app->posts;
    for (int i = 0; i < n; i++) {
        arr[i] = p;
        p = p->next;
    }
    // Urutkan dengan quick sort (descending by likes)
    quick_sort_posts(arr, 0, n-1);
    for (int i = 0; i < 3 && i < n; i++) {
        printf("\n---------------------------------------------------\n");
        printf("[%d] User %d: %s (%s) Likes: %d\n", arr[i]->id, arr[i]->user_id, arr[i]->content, arr[i]->media, arr[i]->likes);
        printf("---------------------------------------------------\n");
    }
    printf("===================================================\n");
    free(arr);
}
void undo_delete_post(AppState *app) {
    if (isUndoEmpty(app)) {
        printf("No post to undo.\n");
        return;
    }
    Post p = popUndo(app);
    if (p.id == 0) {
        printf("No valid post to restore.\n");
        return;
    }
    insert_post(app, p);
    save_posts(app);
    printf("Undo successful. Post restored!\n");
    char notif[MAX_STRING * 2];
    snprintf(notif, sizeof(notif), "You restored post ID %d", p.id);
    enqueueNotif(app, notif);
}

// --- BST Node untuk Post berdasarkan likes ---
typedef struct PostBSTNode {
    Post *post;
    struct PostBSTNode *left, *right;
} PostBSTNode;

PostBSTNode* post_bst_insert(PostBSTNode *root, Post *post) {
    if (!root) {
        PostBSTNode *node = (PostBSTNode*)malloc(sizeof(PostBSTNode));
        node->post = post;
        node->left = node->right = NULL;
        return node;
    }
    if (post->likes < root->post->likes)
        root->left = post_bst_insert(root->left, post);
    else
        root->right = post_bst_insert(root->right, post);
    return root;
}
void post_bst_inorder(PostBSTNode *root) {
    if (!root) return;
    post_bst_inorder(root->left);
    printf("[%d] %s Likes: %d\n", root->post->id, root->post->content, root->post->likes);
    post_bst_inorder(root->right);
}
PostBSTNode* post_bst_search(PostBSTNode *root, int likes) {
    if (!root || root->post->likes == likes) return root;
    if (likes < root->post->likes) return post_bst_search(root->left, likes);
    return post_bst_search(root->right, likes);
}
PostBSTNode* post_bst_find_min(PostBSTNode* root) {
    while (root && root->left) root = root->left;
    return root;
}
PostBSTNode* post_bst_delete(PostBSTNode* root, int likes, int *deleted) {
    if (!root) return NULL;
    if (likes < root->post->likes)
        root->left = post_bst_delete(root->left, likes, deleted);
    else if (likes > root->post->likes)
        root->right = post_bst_delete(root->right, likes, deleted);
    else {
        // Node ditemukan
        *deleted = 1;
        // 1. Leaf node
        if (!root->left && !root->right) {
            free(root);
            return NULL;
        }
        // 2. Satu child
        if (!root->left) {
            PostBSTNode *temp = root->right;
            free(root);
            return temp;
        }
        if (!root->right) {
            PostBSTNode *temp = root->left;
            free(root);
            return temp;
        }
        // 3. Dua children
        PostBSTNode *succ = post_bst_find_min(root->right);
        root->post = succ->post;
        root->right = post_bst_delete(root->right, succ->post->likes, deleted);
    }
    return root;
}
void post_bst_free(PostBSTNode *root) {
    if (!root) return;
    post_bst_free(root->left);
    post_bst_free(root->right);
    free(root);
}
void show_posts_sorted_by_likes(AppState *app) {
    PostBSTNode *bst = NULL;
    Post *p = app->posts;
    while (p) {
        bst = post_bst_insert(bst, p);
        p = p->next;
    }
    printf("Semua post urut likes (inorder BST):\n");
    post_bst_inorder(bst);
    post_bst_free(bst);
}
void search_post_by_likes(AppState *app) {
    int likes;
    printf("Masukkan jumlah likes yang dicari: ");
    scanf("%d", &likes);
    PostBSTNode *bst = NULL;
    Post *p = app->posts;
    while (p) {
        bst = post_bst_insert(bst, p);
        p = p->next;
    }
    PostBSTNode *found = post_bst_search(bst, likes);
    if (found)
        printf("Ditemukan: [%d] %s Likes: %d\n", found->post->id, found->post->content, found->post->likes);
    else
        printf("Tidak ada post dengan likes %d\n", likes);
    post_bst_free(bst);
}

// --- Sorting & Binary Search pada Post ---
void quick_sort_posts(Post **arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high]->likes;
        int i = low - 1;
        for (int j = low; j < high; j++) {
            if (arr[j]->likes > pivot) {
                i++;
                Post *tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
            }
        }
        Post *tmp = arr[i+1]; arr[i+1] = arr[high]; arr[high] = tmp;
        int pi = i+1;
        quick_sort_posts(arr, low, pi-1);
        quick_sort_posts(arr, pi+1, high);
    }
}
void sort_and_show_posts_by_likes(AppState *app) {
    int n = 0;
    Post *p = app->posts;
    while (p) { n++; p = p->next; }
    if (n == 0) {
        printf("No posts.\n");
        return;
    }
    Post **arr = (Post**)malloc(sizeof(Post*) * n);
    p = app->posts;
    for (int i = 0; i < n; i++) {
        arr[i] = p;
        p = p->next;
    }
    quick_sort_posts(arr, 0, n-1);
    printf("Posts sorted by likes (descending):\n");
    for (int i = 0; i < n; i++) {
        printf("[%d] %s Likes: %d\n", arr[i]->id, arr[i]->content, arr[i]->likes);
    }
    free(arr);
}
int binary_search_post(Post **arr, int n, int id) {
    int l = 0, r = n-1;
    while (l <= r) {
        int m = l + (r-l)/2;
        if (arr[m]->id == id) return m;
        if (arr[m]->id < id) l = m+1;
        else r = m-1;
    }
    return -1;
}
void search_post_by_id(AppState *app) {
    int n = 0;
    Post *p = app->posts;
    while (p) { n++; p = p->next; }
    if (n == 0) {
        printf("No posts.\n");
        return;
    }
    Post **arr = (Post**)malloc(sizeof(Post*) * n);
    p = app->posts;
    for (int i = 0; i < n; i++) {
        arr[i] = p;
        p = p->next;
    }
    // Sort by id ascending (bubble sort)
    for (int i = 0; i < n-1; i++)
        for (int j = 0; j < n-i-1; j++)
            if (arr[j]->id > arr[j+1]->id) {
                Post *tmp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = tmp;
            }
    int id;
    printf("Masukkan ID post yang dicari: ");
    scanf("%d", &id);
    int idx = binary_search_post(arr, n, id);
    if (idx != -1)
        printf("Ditemukan: [%d] %s Likes: %d\n", arr[idx]->id, arr[idx]->content, arr[idx]->likes);
    else
        printf("Post dengan ID %d tidak ditemukan.\n", id);
    free(arr);
}
void search_post(AppState *app) {
    int opsi;
    printf("Cari post berdasarkan:\n1. Username\n2. ID\nPilih: ");
    scanf("%d", &opsi);
    getchar(); // flush newline
    if (opsi == 1) {
        char uname[MAX_STRING];
        printf("Masukkan username: ");
        fgets(uname, sizeof(uname), stdin);
        uname[strcspn(uname, "\n")] = 0;
        int uid = -1;
        User *u = app->users;
        while (u) {
            if (strcmp(u->username, uname) == 0) {
                uid = u->id;
                break;
            }
            u = u->next;
        }
        if (uid == -1) {
            printf("Username tidak ditemukan.\n");
            return;
        }
        Post *p = app->posts;
        int found = 0;
        while (p) {
            if (p->user_id == uid) {
                printf("[%d] %s (%s) Likes: %d\n", p->id, p->content, p->media, p->likes);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("Tidak ada post dari user ini.\n");
    } else if (opsi == 2) {
        int n = 0;
        Post *p = app->posts;
        while (p) { n++; p = p->next; }
        if (n == 0) {
            printf("No posts.\n");
            return;
        }
        Post **arr = (Post**)malloc(sizeof(Post*) * n);
        p = app->posts;
        for (int i = 0; i < n; i++) {
            arr[i] = p;
            p = p->next;
        }
        // Sort by id ascending (bubble sort)
        for (int i = 0; i < n-1; i++)
            for (int j = 0; j < n-i-1; j++)
                if (arr[j]->id > arr[j+1]->id) {
                    Post *tmp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = tmp;
                }
        int id;
        printf("Masukkan ID post yang dicari: ");
        scanf("%d", &id);
        int l = 0, r = n-1, idx = -1;
        while (l <= r) {
            int m = l + (r-l)/2;
            if (arr[m]->id == id) { idx = m; break; }
            if (arr[m]->id < id) l = m+1;
            else r = m-1;
        }
        if (idx != -1)
            printf("Ditemukan: [%d] %s Likes: %d\n", arr[idx]->id, arr[idx]->content, arr[idx]->likes);
        else
            printf("Post dengan ID %d tidak ditemukan.\n", id);
        free(arr);
    } else {
        printf("Pilihan tidak valid.\n");
    }
}

// --- Menu ---
// Function prototype for pop_top_liked_post
void pop_top_liked_post(AppState *app);
// Function prototype for unlike_post_bst
void unlike_post_bst(AppState *app);

// Function prototype for delete_bst_node_menu
void delete_bst_node_menu(AppState *app);

// Function prototype for user_menu
void user_menu(AppState *app);

void main_menu(AppState *app) {
    int choice;
    do {
        printf("\n=====================================================\n");
        printf("                SELAMAT DATANG DI INSTAGRAM          \n");
        printf("=====================================================\n");
        printf("  1. Sign Up\n");
        printf("  2. Log In\n");
        printf("  3. Exit\n");
        printf("-----------------------------------------------------\n");
        printf("Pilih menu (1-3): ");
        scanf("%d", &choice);
        printf("=====================================================\n");
        switch (choice) {
            case 1: signup(app); break;
            case 2:
                app->current_user_id = login(app);
                if (app->current_user_id != -1) user_menu(app);
                break;
            case 3: 
                printf("\nTerima kasih telah menggunakan aplikasi!\n\n");
                exit(0);
            default: printf(">> Pilihan tidak valid!\n");
        }
    } while (1);
}

void user_menu(AppState *app) {
    int choice;
    do {
        printf("\n=====================================================\n");
        printf("                    USER MENU                        \n");
        printf("=====================================================\n");
        printf("  1.  Create Post\n");
        printf("  2.  View Posts\n");
        printf("  3.  Like Post\n");
        printf("  4.  Unlike Post\n");
        printf("  5.  Comment Post\n");
        printf("  6.  Delete Post\n");
        printf("  7.  Edit Post\n");
        printf("  8.  Search Post\n");
        printf("  9.  View Posts by Likes\n");
        printf(" 10.  Undo Delete Post\n");
        printf(" 11.  Show Notifications\n");
        printf(" 12.  Top Liked Posts (Heap)\n");
        printf(" 13.  Hapus Top Liked Post (Heap)\n");
        printf(" 14.  Tampilkan Post BST (Inorder)\n");
        printf(" 15.  Cari Post by Likes (BST)\n");
        printf(" 16.  Unlike Post (BST)\n");
        printf(" 17.  Hapus Node BST\n");
        printf(" 18.  Log Out\n");
        printf("-----------------------------------------------------\n");
        printf("Pilih menu (1-18): ");
        scanf("%d", &choice);
        printf("=====================================================\n");
        switch (choice) {
            case 1: create_post(app); break;
            case 2: view_posts(app); break;
            case 3: like_post(app); break;
            case 4: unlike_post(app); break;
            case 5: comment_post(app); break;
            case 6: delete_post(app); break;
            case 7: edit_post(app); break;
            case 8: search_post(app); break;
            case 9: sort_and_show_posts_by_likes(app); break;
            case 10: undo_delete_post(app); break;
            case 11: showNotifications(app); break;
            case 12: top_liked_posts(app); break;
            case 13: pop_top_liked_post(app); break;
            case 14: show_posts_sorted_by_likes(app); break;
            case 15: search_post_by_likes(app); break;
            case 16: unlike_post_bst(app); break;
            case 17: delete_bst_node_menu(app); break;
            case 18: app->current_user_id = -1; return;
            default: printf(">> Pilihan tidak valid!\n");
        }
    } while (1);
}
void free_all(AppState *app) {
    // Free users
    User *u = app->users;
    while (u) {
        User *tmp = u;
        u = u->next;
        free(tmp);
    }
    // Free posts
    Post *p = app->posts;
    while (p) {
        Post *tmp = p;
        p = p->next;
        free(tmp);
    }
    // Free comments
    Comment *c = app->comments;
    while (c) {
        Comment *tmp = c;
        c = c->next;
        free(tmp);
    }
    // Free undo stack
    UndoNode *un = app->undoTop;
    while (un) {
        UndoNode *tmp = un;
        un = un->next;
        free(tmp);
    }
    // Free notifications queue
    NotifNode *nn = app->notifFront;
    while (nn) {
        NotifNode *tmp = nn;
        nn = nn->next;
        free(tmp);
    }
}

int main() {
    AppState app = {0};
    app.users = NULL;
    app.posts = NULL;
    app.comments = NULL;
    app.undoTop = NULL;
    app.notifFront = app.notifRear = NULL;
    load_users(&app);
    load_posts(&app);
    load_comments(&app);
    main_menu(&app);
    free_all(&app);
    return 0;
}
// Tambahkan ke menu heap Anda
void pop_top_liked_post(AppState *app) {
    int n = 0;
    Post *p = app->posts;
    while (p) { n++; p = p->next; }
    if (n == 0) {
        printf("\n>> Tidak ada post.\n");
        return;
    }
    Post **arr = (Post**)malloc(sizeof(Post*) * n);
    p = app->posts;
    for (int i = 0; i < n; i++) {
        arr[i] = p;
        p = p->next;
    }
    // Urutkan dengan quick sort (descending by likes)
    quick_sort_posts(arr, 0, n-1);

    // Cari top liked post milik user sendiri
    Post *top = NULL;
    for (int i = 0; i < n; i++) {
        if (arr[i]->user_id == app->current_user_id) {
            top = arr[i];
            break;
        }
    }
    if (!top) {
        printf("\n>> Tidak ada post Anda di daftar!\n");
        free(arr);
        return;
    }
    printf("\nMenghapus post dengan likes terbanyak milik Anda:\n");
    printf("--------------------------------------------------\n");
    printf("[%d] %s (%s) Likes: %d\n", top->id, top->content, top->media, top->likes);
    printf("--------------------------------------------------\n");

    // Hapus dari linked list
    Post **pp = &app->posts;
    while (*pp) {
        if (*pp == top) {
            Post *del = *pp;
            *pp = del->next;
            free(del);
            app->post_count--;
            break;
        }
        pp = &(*pp)->next;
    }
    save_posts(app);
    free(arr);
}

// Unlike Post (BST)
void unlike_post_bst(AppState *app) {
    int likes;
    printf("Masukkan jumlah likes post yang ingin di-unlike (BST): ");
    scanf("%d", &likes);

    // Bangun BST dari linked list post
    PostBSTNode *bst = NULL;
    Post *p = app->posts;
    while (p) {
        bst = post_bst_insert(bst, p);
        p = p->next;
    }

    // Cari node BST dengan jumlah likes yang diinput
    PostBSTNode *found = post_bst_search(bst, likes);
    if (found && found->post->likes > 0) {
        found->post->likes--;
        save_posts(app);
        printf("Unlike berhasil! Post [%d] sekarang likes: %d\n", found->post->id, found->post->likes);
        char notif[MAX_STRING * 2];
        snprintf(notif, sizeof(notif), "You unliked post ID %d (BST)", found->post->id);
        enqueueNotif(app, notif);
    } else if (found && found->post->likes == 0) {
        printf("Likes sudah 0, tidak bisa di-unlike lagi.\n");
    } else {
        printf("Tidak ada post dengan likes %d\n", likes);
    }
    post_bst_free(bst);
}

// Menu untuk menghapus node BST berdasarkan likes
void delete_bst_node_menu(AppState *app) {
    int likes;
    printf("Masukkan jumlah likes node BST yang ingin dihapus: ");
    scanf("%d", &likes);

    // Bangun BST dari linked list post
    PostBSTNode *bst = NULL;
    Post *p = app->posts;
    while (p) {
        bst = post_bst_insert(bst, p);
        p = p->next;
    }

    int deleted = 0;
    bst = post_bst_delete(bst, likes, &deleted);
    if (deleted) {
        printf("Node dengan likes %d berhasil dihapus dari BST (hanya di BST, data asli tidak berubah).\n", likes);
    } else {
        printf("Node dengan likes %d tidak ditemukan di BST.\n", likes);
    }
    post_bst_free(bst);
}