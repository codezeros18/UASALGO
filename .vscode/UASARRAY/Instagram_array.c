#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_STRING 100
#define MAX_USER 100
#define MAX_POST 1000
#define MAX_COMMENT 1000
#define MAX_UNDO 100
#define MAX_NOTIF 100

typedef struct {
    int id;
    char username[MAX_STRING];
    char email[MAX_STRING];
    char password[MAX_STRING];
} User;

typedef struct {
    int id;
    int user_id;
    char content[MAX_STRING];
    char media[MAX_STRING];
    int likes;
} Post;

typedef struct {
    int id;
    int post_id;
    int user_id;
    char text[MAX_STRING];
} Comment;

// Stack for Undo (array)
typedef struct {
    Post stack[MAX_UNDO];
    int top;
} UndoStack;

// Queue for Notifications (array)
typedef struct {
    char msg[MAX_NOTIF][MAX_STRING * 2];
    int front, rear, count;
} NotifQueue;

typedef struct {
    User users[MAX_USER];
    Post posts[MAX_POST];
    Comment comments[MAX_COMMENT];
    int user_count;
    int post_count;
    int comment_count;
    int current_user_id;
    int last_post_id;

    UndoStack undoStack;
    NotifQueue notifQueue;
} AppState;

// --- Stack & Queue Array Version ---
void pushUndo(AppState *app, Post p) {
    if (app->undoStack.top < MAX_UNDO - 1) {
        app->undoStack.stack[++app->undoStack.top] = p;
    }
}
Post popUndo(AppState *app) {
    if (app->undoStack.top >= 0)
        return app->undoStack.stack[app->undoStack.top--];
    Post empty = {0};
    return empty;
}
int isUndoEmpty(AppState *app) {
    return app->undoStack.top == -1;
}
void enqueueNotif(AppState *app, const char *msg) {
    if (app->notifQueue.count < MAX_NOTIF) {
        strcpy(app->notifQueue.msg[app->notifQueue.rear], msg);
        app->notifQueue.rear = (app->notifQueue.rear + 1) % MAX_NOTIF;
        app->notifQueue.count++;
    }
}
void showNotifications(AppState *app) {
    printf("\n==================[ Notifications ]==================\n");
    if (app->notifQueue.count == 0) {
        printf(">> Tidak ada notifikasi.\n");
    } else {
        int idx = app->notifQueue.front;
        for (int i = 0; i < app->notifQueue.count; i++) {
            printf("%d. %s\n", i + 1, app->notifQueue.msg[idx]);
            idx = (idx + 1) % MAX_NOTIF;
        }
    }
    printf("=====================================================\n");
}

// --- Array Insert/Delete/Search mirip Linked List ---
void insert_user(AppState *app, User u) {
    if (app->user_count < MAX_USER) {
        app->users[app->user_count++] = u;
    }
}
void insert_post(AppState *app, Post p) {
    if (app->post_count < MAX_POST) {
        app->posts[app->post_count++] = p;
    }
}
void insert_comment(AppState *app, Comment c) {
    if (app->comment_count < MAX_COMMENT) {
        app->comments[app->comment_count++] = c;
    }
}

// Delete post by ID (array, mirip linked list)
void delete_post(AppState *app) {
    int pid;
    printf("Enter post ID to delete: ");
    scanf("%d", &pid);
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].id == pid && app->posts[i].user_id == app->current_user_id) {
            pushUndo(app, app->posts[i]);
            // Geser array ke kiri
            for (int j = i; j < app->post_count - 1; j++) {
                app->posts[j] = app->posts[j + 1];
            }
            app->post_count--;
            printf("Post deleted. (Undo available)\n");
            char notif[MAX_STRING * 2];
            snprintf(notif, sizeof(notif), "You deleted post ID %d", pid);
            enqueueNotif(app, notif);
            return;
        }
    }
    printf("Post not found or you are not the owner.\n");
}

// Undo delete post (array)
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
    printf("Undo successful. Post restored!\n");
    char notif[MAX_STRING * 2];
    snprintf(notif, sizeof(notif), "You restored post ID %d", p.id);
    enqueueNotif(app, notif);
}

// View posts (array, mirip linked list)
void view_posts(AppState *app) {
    printf("\n====================[ Daftar Postingan ]====================\n");
    int ada = 0;
    for (int i = 0; i < app->post_count; i++) {
        Post *p = &app->posts[i];
        ada = 1;
        printf("\n------------------------------------------------------------\n");
        printf("[%d] User %d: %s (%s) Likes: %d\n", p->id, p->user_id, p->content, p->media, p->likes);
        // Print comments (reverse order)
        for (int j = app->comment_count - 1; j >= 0; j--) {
            if (app->comments[j].post_id == p->id) {
                printf("  - Comment from User %d: %s\n", app->comments[j].user_id, app->comments[j].text);
            }
        }
    }
    if (!ada) printf("\n>> Belum ada postingan.\n");
    printf("\n============================================================\n");
}

// Save users to file (simple implementation)
void save_users(AppState *app) {
    FILE *f = fopen("users.txt", "w");
    if (!f) return;
    for (int i = 0; i < app->user_count; i++) {
        fprintf(f, "%d|%s|%s|%s\n", app->users[i].id, app->users[i].username, app->users[i].email, app->users[i].password);
    }
    fclose(f);
}

// Save posts to file (simple implementation)
void save_posts(AppState *app) {
    FILE *f = fopen("posts.txt", "w");
    if (!f) return;
    for (int i = 0; i < app->post_count; i++) {
        fprintf(f, "%d|%d|%s|%s|%d\n", app->posts[i].id, app->posts[i].user_id, app->posts[i].content, app->posts[i].media, app->posts[i].likes);
    }
    fclose(f);
}

void save_comments(AppState *app) {
    FILE *f = fopen("comments.txt", "w");
    if (!f) return;
    for (int i = 0; i < app->comment_count; i++) {
        fprintf(f, "%d|%d|%d|%s\n", app->comments[i].id, app->comments[i].post_id, app->comments[i].user_id, app->comments[i].text);
    }
    fclose(f);
}

// Load users from file
void load_users(AppState *app) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return;
    User u;
    while (fscanf(f, "%d|%[^|]|%[^|]|%s\n", &u.id, u.username, u.email, u.password) == 4) {
        insert_user(app, u);
    }
    fclose(f);
}

void load_posts(AppState *app) {
    FILE *f = fopen("posts.txt", "r");
    if (!f) return;
    Post p;
    int max_id = 0;
    while (fscanf(f, "%d|%d|%[^|]|%[^|]|%d\n", &p.id, &p.user_id, p.content, p.media, &p.likes) == 5) {
        insert_post(app, p);
        if (p.id > max_id) max_id = p.id;
    }
    app->last_post_id = max_id;
    fclose(f);
}

void load_comments(AppState *app) {
    FILE *f = fopen("comments.txt", "r");
    if (!f) return;
    Comment c;
    while (fscanf(f, "%d|%d|%d|%[^\n]\n", &c.id, &c.post_id, &c.user_id, c.text) == 4) {
        insert_comment(app, c);
    }
    fclose(f);
}

// Signup
int signup(AppState *app) {
    User u;
    u.id = app->user_count + 1;
    printf("Username: ");
    scanf(" %[^\n]", u.username);
    printf("Email: ");
    scanf(" %[^\n]", u.email);
    printf("Password: ");
    scanf(" %[^\n]", u.password);
    insert_user(app, u);
    save_users(app);
    printf("Signup successful. Please login.\n");
    return -1;
}

// Login
int login(AppState *app) {
    char uname[MAX_STRING], pass[MAX_STRING];
    printf("Username: ");
    scanf(" %[^\n]", uname);
    printf("Password: ");
    scanf(" %[^\n]", pass);
    for (int i = 0; i < app->user_count; i++) {
        if (strcmp(app->users[i].username, uname) == 0 && strcmp(app->users[i].password, pass) == 0) {
            printf("Login successful!\n");
            return app->users[i].id;
        }
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
    insert_post(app, p);
    save_posts(app);
    printf("Post created.\n");
}

// Like post
void like_post(AppState *app) {
    int pid;
    printf("Enter post ID to like: ");
    scanf("%d", &pid);
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].id == pid) {
            app->posts[i].likes++;
            save_posts(app);
            printf("Post liked!\n");
            char notif[MAX_STRING * 2];
            snprintf(notif, sizeof(notif), "You liked post ID %d", app->posts[i].id);
            enqueueNotif(app, notif);
            return;
        }
    }
    printf("Post not found.\n");
}

// Unlike post
void unlike_post(AppState *app) {
    int pid;
    printf("Enter post ID to unlike: ");
    scanf("%d", &pid);
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].id == pid && app->posts[i].user_id == app->current_user_id) {
            if (app->posts[i].likes > 0) {
                app->posts[i].likes--;
                save_posts(app);
                printf("Post unliked!\n");
                char notif[MAX_STRING * 2];
                snprintf(notif, sizeof(notif), "You unliked post ID %d", app->posts[i].id);
                enqueueNotif(app, notif);
            } else {
                printf("Post already has 0 likes.\n");
            }
            return;
        }
    }
    printf("Post not found or you are not the owner.\n");
}

// Comment post
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
    insert_comment(app, c);
    save_comments(app);
    printf("Comment added.\n");
    char notif[MAX_STRING * 2];
    snprintf(notif, sizeof(notif), "You commented on post ID %d", pid);
    enqueueNotif(app, notif);
}

// Edit post (array, mirip linked list)
void edit_post(AppState *app) {
    int pid;
    printf("Enter post ID to edit: ");
    scanf("%d", &pid);
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].id == pid && app->posts[i].user_id == app->current_user_id) {
            printf("New Media Filename (png, jpg, etc): ");
            scanf(" %[^\n]", app->posts[i].media);
            printf("New Caption: ");
            scanf(" %[^\n]", app->posts[i].content);
            save_posts(app);
            printf("Post updated.\n");
            return;
        }
    }
    printf("Post not found or unauthorized.\n");
}

// Search post by username (array, mirip linked list)
void search_by_username(AppState *app) {
    char uname[MAX_STRING];
    printf("Enter username to search posts: ");
    scanf(" %[^\n]", uname);
    int uid = -1;
    for (int i = 0; i < app->user_count; i++) {
        if (strcmp(app->users[i].username, uname) == 0) {
            uid = app->users[i].id;
            break;
        }
    }
    if (uid == -1) {
        printf("Username not found.\n");
        return;
    }
    int found = 0;
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].user_id == uid) {
            printf("[%d] %s (%s) Likes: %d\n", app->posts[i].id, app->posts[i].content, app->posts[i].media, app->posts[i].likes);
            found = 1;
        }
    }
    if (!found) printf("No posts from this user.\n");
}

// View own posts (array, mirip linked list)
void view_own_posts(AppState *app) {
    printf("\n=================[ Postingan Anda ]==================\n");
    int ada = 0;
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].user_id == app->current_user_id) {
            ada = 1;
            printf("[%d] %s (%s) Likes: %d\n", app->posts[i].id, app->posts[i].content, app->posts[i].media, app->posts[i].likes);
        }
    }
    if (!ada) printf(">> Anda belum membuat postingan.\n");
    printf("=====================================================\n");
}

// Sorting posts by likes (Quick Sort)
void quick_sort_posts(Post *arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high].likes;
        int i = low - 1;
        for (int j = low; j < high; j++) {
            if (arr[j].likes > pivot) {
                i++;
                Post tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
            }
        }
        Post tmp = arr[i+1]; arr[i+1] = arr[high]; arr[high] = tmp;
        int pi = i+1;
        quick_sort_posts(arr, low, pi-1);
        quick_sort_posts(arr, pi+1, high);
    }
}

// Show posts sorted by likes (descending)
void sort_and_show_posts_by_likes(AppState *app) {
    if (app->post_count == 0) {
        printf("No posts.\n");
        return;
    }
    Post *arr = (Post*)malloc(sizeof(Post) * app->post_count);
    for (int i = 0; i < app->post_count; i++) arr[i] = app->posts[i];
    quick_sort_posts(arr, 0, app->post_count-1);
    printf("Posts sorted by likes (descending):\n");
    for (int i = 0; i < app->post_count; i++) {
        printf("[%d] %s Likes: %d\n", arr[i].id, arr[i].content, arr[i].likes);
    }
    free(arr);
}

// Binary search post by ID (array)
int binary_search_post(Post *arr, int n, int id) {
    int l = 0, r = n-1;
    while (l <= r) {
        int m = l + (r-l)/2;
        if (arr[m].id == id) return m;
        if (arr[m].id < id) l = m+1;
        else r = m-1;
    }
    return -1;
}

// Search post by ID (array)
void search_post_by_id(AppState *app) {
    if (app->post_count == 0) {
        printf("No posts.\n");
        return;
    }
    // Sort by id ascending (bubble sort)
    Post *arr = (Post*)malloc(sizeof(Post) * app->post_count);
    for (int i = 0; i < app->post_count; i++) arr[i] = app->posts[i];
    for (int i = 0; i < app->post_count-1; i++)
        for (int j = 0; j < app->post_count-i-1; j++)
            if (arr[j].id > arr[j+1].id) {
                Post tmp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = tmp;
            }
    int id;
    printf("Masukkan ID post yang dicari: ");
    scanf("%d", &id);
    int idx = binary_search_post(arr, app->post_count, id);
    if (idx != -1)
        printf("Ditemukan: [%d] %s Likes: %d\n", arr[idx].id, arr[idx].content, arr[idx].likes);
    else
        printf("Post dengan ID %d tidak ditemukan.\n", id);
    free(arr);
}

// Top 3 liked posts (Heap/Sort)
void top_liked_posts(AppState *app) {
    printf("\n==================[ Top 3 Posts ]==================\n");
    if (app->post_count == 0) {
        printf("\n>> Tidak ada post.\n");
        printf("===================================================\n");
        return;
    }
    Post *arr = (Post*)malloc(sizeof(Post) * app->post_count);
    for (int i = 0; i < app->post_count; i++) arr[i] = app->posts[i];
    quick_sort_posts(arr, 0, app->post_count-1);
    for (int i = 0; i < 3 && i < app->post_count; i++) {
        printf("\n---------------------------------------------------\n");
        printf("[%d] User %d: %s (%s) Likes: %d\n", arr[i].id, arr[i].user_id, arr[i].content, arr[i].media, arr[i].likes);
        printf("---------------------------------------------------\n");
    }
    printf("===================================================\n");
    free(arr);
}

// Pop top liked post (Heap/Sort)
void pop_top_liked_post(AppState *app) {
    if (app->post_count == 0) {
        printf("\n>> Tidak ada post.\n");
        return;
    }
    Post *arr = (Post*)malloc(sizeof(Post) * app->post_count);
    for (int i = 0; i < app->post_count; i++) arr[i] = app->posts[i];
    quick_sort_posts(arr, 0, app->post_count-1);
    // Cari top liked post milik user sendiri
    int idx = -1;
    for (int i = 0; i < app->post_count; i++) {
        if (arr[i].user_id == app->current_user_id) {
            idx = arr[i].id;
            break;
        }
    }
    if (idx == -1) {
        printf("\n>> Tidak ada post Anda di daftar!\n");
        free(arr);
        return;
    }
    printf("\nMenghapus post dengan likes terbanyak milik Anda:\n");
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].id == idx) {
            printf("[%d] %s (%s) Likes: %d\n", app->posts[i].id, app->posts[i].content, app->posts[i].media, app->posts[i].likes);
            // Hapus dari array
            for (int j = i; j < app->post_count-1; j++)
                app->posts[j] = app->posts[j+1];
            app->post_count--;
            save_posts(app);
            break;
        }
    }
    free(arr);
}

// ==================== BST FITUR ====================
// BST Node untuk Post berdasarkan likes
typedef struct BSTNode {
    Post post;
    struct BSTNode *left, *right;
} BSTNode;

// Helper: Insert post ke BST berdasarkan likes
BSTNode* insert_bst(BSTNode* root, Post p) {
    if (!root) {
        BSTNode* node = (BSTNode*)malloc(sizeof(BSTNode));
        node->post = p;
        node->left = node->right = NULL;
        return node;
    }
    if (p.likes < root->post.likes)
        root->left = insert_bst(root->left, p);
    else
        root->right = insert_bst(root->right, p);
    return root;
}

// Helper: Inorder traversal BST
void inorder_bst(BSTNode* root) {
    if (!root) return;
    inorder_bst(root->left);
    printf("[%d] %s (%s) Likes: %d\n", root->post.id, root->post.content, root->post.media, root->post.likes);
    inorder_bst(root->right);
}

// Free BST
void free_bst(BSTNode* node) {
    if (!node) return;
    free_bst(node->left);
    free_bst(node->right);
    free(node);
}

// Fitur: Tampilkan post BST (Inorder)
void tampilkan_post_bst_inorder(AppState *app) {
    if (app->post_count == 0) {
        printf("Tidak ada post.\n");
        return;
    }
    BSTNode* root = NULL;
    for (int i = 0; i < app->post_count; i++) {
        root = insert_bst(root, app->posts[i]);
    }
    printf("Post (Inorder BST by Likes):\n");
    inorder_bst(root);
    free_bst(root);
}

// Fitur: Cari post di BST berdasarkan jumlah likes
void cari_post_bst_by_likes(AppState *app) {
    if (app->post_count == 0) {
        printf("Tidak ada post.\n");
        return;
    }
    int target_likes;
    printf("Masukkan jumlah likes yang dicari: ");
    scanf("%d", &target_likes);
    BSTNode* root = NULL;
    for (int i = 0; i < app->post_count; i++) {
        root = insert_bst(root, app->posts[i]);
    }
    // Cari dan tampilkan semua post dengan likes == target_likes
    int found = 0;
    void search_bst(BSTNode* node, int likes) {
        if (!node) return;
        search_bst(node->left, likes);
        if (node->post.likes == likes) {
            printf("[%d] %s (%s) Likes: %d\n", node->post.id, node->post.content, node->post.media, node->post.likes);
            found = 1;
        }
        search_bst(node->right, likes);
    }
    search_bst(root, target_likes);
    if (!found) printf("Tidak ada post dengan %d likes.\n", target_likes);
    free_bst(root);
}

// Unlike post in BST by likes (decrement likes for a post with given likes owned by current user)
void unlike_post_bst(AppState *app) {
    if (app->post_count == 0) {
        printf("Tidak ada post.\n");
        return;
    }
    int target_likes;
    printf("Masukkan jumlah likes post yang ingin di-unlike: ");
    scanf("%d", &target_likes);
    int found = 0;
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].likes == target_likes && app->posts[i].user_id == app->current_user_id) {
            if (app->posts[i].likes > 0) {
                app->posts[i].likes--;
                save_posts(app);
                printf("Unlike berhasil pada post ID %d.\n", app->posts[i].id);
                char notif[MAX_STRING * 2];
                snprintf(notif, sizeof(notif), "You unliked post ID %d (BST)", app->posts[i].id);
                enqueueNotif(app, notif);
            } else {
                printf("Post sudah memiliki 0 likes.\n");
            }
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Tidak ditemukan post Anda dengan %d likes.\n", target_likes);
    }
}

// Hapus node BST (menghapus post dengan jumlah likes tertentu milik user saat ini)
void hapus_node_bst(AppState *app) {
    if (app->post_count == 0) {
        printf("Tidak ada post.\n");
        return;
    }
    int target_likes;
    printf("Masukkan jumlah likes post yang ingin dihapus: ");
    scanf("%d", &target_likes);
    int found = 0;
    for (int i = 0; i < app->post_count; i++) {
        if (app->posts[i].likes == target_likes && app->posts[i].user_id == app->current_user_id) {
            printf("Menghapus post ID %d dengan %d likes.\n", app->posts[i].id, app->posts[i].likes);
            // Hapus dari array
            for (int j = i; j < app->post_count - 1; j++) {
                app->posts[j] = app->posts[j + 1];
            }
            app->post_count--;
            save_posts(app);
            char notif[MAX_STRING * 2];
            snprintf(notif, sizeof(notif), "You deleted post ID %d (BST)", app->posts[i].id);
            enqueueNotif(app, notif);
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Tidak ditemukan post Anda dengan %d likes.\n", target_likes);
    }
}

// ==================== MENU ====================
void user_menu(AppState *app) {
    int choice;
    do {
        printf("\n======================== USER MENU ========================\n");
        printf("  1.  Create Post\n");
        printf("  2.  View Posts\n");
        printf("  3.  Like Post\n");
        printf("  4.  Unlike Post\n");
        printf("  5.  Comment Post\n");
        printf("  6.  Delete Post\n");
        printf("  7.  Edit Post\n");
        printf("  8.  Search Post by ID\n");
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
        printf("-----------------------------------------------------------\n");
        printf("Pilih menu (1-18): ");
        scanf("%d", &choice);
        printf("===========================================================\n");
        switch (choice) {
            case 1: create_post(app); break;
            case 2: view_posts(app); break;
            case 3: like_post(app); break;
            case 4: unlike_post(app); break;
            case 5: comment_post(app); break;
            case 6: delete_post(app); break;
            case 7: edit_post(app); break;
            case 8: search_post_by_id(app); break;
            case 9: sort_and_show_posts_by_likes(app); break;
            case 10: undo_delete_post(app); break;
            case 11: showNotifications(app); break;
            case 12: top_liked_posts(app); break;
            case 13: pop_top_liked_post(app); break;
            case 14: tampilkan_post_bst_inorder(app); break;
            case 15: cari_post_bst_by_likes(app); break;
            case 16: unlike_post_bst(app); break;
            case 17: hapus_node_bst(app); break;
            case 18: app->current_user_id = -1; return;
            default: printf(">> Pilihan tidak valid!\n");
        }
    } while (1);
}

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

int main() {
    AppState app = {0};
    app.undoStack.top = -1;
    app.notifQueue.front = 0;
    app.notifQueue.rear = 0;
    app.notifQueue.count = 0;
    app.user_count = 0;
    app.post_count = 0;
    app.comment_count = 0;
    app.current_user_id = -1;
    app.last_post_id = 0;
    load_users(&app); // Tambahkan baris ini
    load_posts(&app);
    load_comments(&app);
    main_menu(&app);
    return 0;
}
