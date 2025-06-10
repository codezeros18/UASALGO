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

typedef struct LikeNode {
    int user_id;
    struct LikeNode *next;
} LikeNode;

typedef struct Post {
    int id;
    int user_id;
    char content[MAX_STRING];
    char media[MAX_STRING];
    int likes;
    struct Post *next;
    LikeNode *likeList; // Tambahkan ini
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

// Tambahkan struct BST untuk User
typedef struct UserBSTNode {
    struct User *user;
    struct UserBSTNode *left, *right;
} UserBSTNode;

// Tambahkan struct BST untuk Comment
typedef struct CommentBSTNode {
    struct Comment *comment;
    struct CommentBSTNode *left, *right;
} CommentBSTNode;

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

    // Tambahkan di AppState:
    UserBSTNode *userBST; // Tambahkan pointer ke root BST User
    CommentBSTNode *commentBST; // Tambahkan pointer ke root BST Comment
} AppState;

// ======================= BST untuk Post =========================
typedef struct PostBSTNode {
    Post *post;
    struct PostBSTNode *left, *right;
} PostBSTNode;


// [BST] Insert node ke BST Post
PostBSTNode* insert_post_bst(PostBSTNode *root, Post *post) {
    if (!root) {
        PostBSTNode *node = (PostBSTNode*)malloc(sizeof(PostBSTNode));
        node->post = post;
        node->left = node->right = NULL;
        return node;
    }
    if (post->id < root->post->id)
        root->left = insert_post_bst(root->left, post);
    else if (post->id > root->post->id)
        root->right = insert_post_bst(root->right, post);
    return root;
}

// [BST] Cari node pada BST Post
Post* search_post_bst(PostBSTNode *root, int id) {
    if (!root) return NULL;
    if (id == root->post->id) return root->post;
    if (id < root->post->id) return search_post_bst(root->left, id);
    else return search_post_bst(root->right, id);
}

// [BST] Cari node minimum pada BST Post
PostBSTNode* find_min_bst(PostBSTNode *root) {
    while (root && root->left) root = root->left;
    return root;
}

// [BST] Hapus node pada BST Post
PostBSTNode* delete_post_bst(PostBSTNode *root, int id) {
    if (!root) return NULL;
    if (id < root->post->id)
        root->left = delete_post_bst(root->left, id);
    else if (id > root->post->id)
        root->right = delete_post_bst(root->right, id);
    else {
        if (!root->left && !root->right) { // Leaf
            free(root);
            return NULL;
        } else if (!root->left || !root->right) { // 1 child
            PostBSTNode *child = root->left ? root->left : root->right;
            free(root);
            return child;
        } else { // 2 children
            PostBSTNode *succ = find_min_bst(root->right);
            root->post = succ->post;
            root->right = delete_post_bst(root->right, succ->post->id);
        }
    }
    return root;
}

// [BST] Bebaskan seluruh node BST Post
void free_post_bst(PostBSTNode *root) {
    if (!root) return;
    free_post_bst(root->left);
    free_post_bst(root->right);
    free(root);
}

// [BST] Build BST dari linked list Post
PostBSTNode* build_post_bst(Post *head) {
    PostBSTNode *root = NULL;
    while (head) {
        root = insert_post_bst(root, head);
        head = head->next;
    }
    return root;
}

// ======================= Heap untuk Likes =========================
typedef struct {
    Post **arr;
    int size;
    int capacity;
} PostHeap;

// [Heap] Heapify array Post berdasarkan likes
void heapify(PostHeap *heap, int i) {
    int largest = i;
    int l = 2*i+1, r = 2*i+2;
    if (l < heap->size && heap->arr[l]->likes > heap->arr[largest]->likes)
        largest = l;
    if (r < heap->size && heap->arr[r]->likes > heap->arr[largest]->likes)
        largest = r;
    if (largest != i) {
        Post *tmp = heap->arr[i];
        heap->arr[i] = heap->arr[largest];
        heap->arr[largest] = tmp;
        heapify(heap, largest);
    }
}

// [Heap] Build max-heap dari linked list Post
void build_post_heap(PostHeap *heap, Post *head) {
    int n = 0;
    Post *p = head;
    while (p) { n++; p = p->next; }
    heap->arr = (Post**)malloc(sizeof(Post*) * n);
    heap->size = n;
    heap->capacity = n;
    p = head;
    for (int i = 0; i < n; i++) {
        heap->arr[i] = p;
        p = p->next;
    }
    for (int i = n/2-1; i >= 0; i--)
        heapify(heap, i);
}

// [Heap] Ambil elemen max (likes terbanyak) dari heap
Post* extract_max(PostHeap *heap) {
    if (heap->size == 0) return NULL;
    Post *max = heap->arr[0];
    heap->arr[0] = heap->arr[--heap->size];
    heapify(heap, 0);
    return max;
}

// [Heap] Bebaskan array heap
void free_post_heap(PostHeap *heap) {
    free(heap->arr);
}

// ======================= Stack & Queue =========================

// [Stack] Push ke undo stack (linked list)
void pushUndo(AppState *app, Post p) {
    if (p.id == 0) return;
    UndoNode *node = (UndoNode*)malloc(sizeof(UndoNode));
    node->post = p;
    node->next = app->undoTop;
    app->undoTop = node;
}

// [Stack] Pop dari undo stack
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

// [Stack] Cek apakah undo stack kosong
int isUndoEmpty(AppState *app) {
    return app->undoTop == NULL;
}

// [Queue] Enqueue notifikasi ke queue (linked list)
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

// [Queue] Tampilkan seluruh notifikasi (dequeue)
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

// [Linked List] Insert post ke linked list
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

// [File I/O] Load posts dari file ke linked list
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

// [File I/O] Load comments dari file ke linked list
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

// [File I/O] Save users ke file
void save_users(AppState *app) {
    FILE *file = fopen("users.txt", "w");
    User *u = app->users;
    while (u) {
        fprintf(file, "%d|%s|%s|%s\n", u->id, u->username, u->email, u->password);
        u = u->next;
    }
    fclose(file);
}

// [File I/O] Save posts ke file
void save_posts(AppState *app) {
    FILE *file = fopen("posts.txt", "w");
    Post *p = app->posts;
    while (p) {
        fprintf(file, "%d|%d|%s|%s|%d\n", p->id, p->user_id, p->content, p->media, p->likes);
        p = p->next;
    }
    fclose(file);
}

// [File I/O] Save comments ke file
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
// Function prototype for insert_user_bst
UserBSTNode* insert_user_bst(UserBSTNode *root, User *user);

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
    // Tambahkan baris berikut agar BST user ikut di-update
    app->userBST = insert_user_bst(app->userBST, app->users); // app->users adalah user baru di head
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

// Function prototype for search_user_bst
User* search_user_bst(UserBSTNode *root, const char *username);

// Login
int login(AppState *app) {
    char uname[MAX_STRING], pass[MAX_STRING];
    printf("Username: ");
    scanf(" %[^\n]", uname);
    printf("Password: ");
    scanf(" %[^\n]", pass);
    User *u = search_user_bst(app->userBST, uname);
    if (u && strcmp(u->password, pass) == 0) {
        printf("Login successful!\n");
        char logmsg[MAX_STRING * 2];
        snprintf(logmsg, sizeof(logmsg), "User %s logged in.", uname);
        log_activity(logmsg);
        return u->id;
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
// Function prototype for bubble_sort_posts_by_id
void bubble_sort_posts_by_id(Post **arr, int n);

void view_posts(AppState *app) {
    int n = 0;
    Post *p = app->posts;
    while (p) { n++; p = p->next; }
    if (n == 0) {
        printf("\n>> Belum ada postingan.\n");
        return;
    }
    Post **arr = (Post**)malloc(sizeof(Post*) * n);
    p = app->posts;
    for (int i = 0; i < n; i++) {
        arr[i] = p;
        p = p->next;
    }
    bubble_sort_posts_by_id(arr, n);
    printf("\n====================[ Daftar Postingan ]====================\n");
    for (int i = 0; i < n; i++) {
        printf("\n------------------------------------------------------------\n");
        printf("[%d] User %d: %s (%s) Likes: %d\n", arr[i]->id, arr[i]->user_id, arr[i]->content, arr[i]->media, arr[i]->likes);
        // Print comments (reverse order)
        int stack_count = 0;
        Comment *stack[1000];
        Comment *c = app->comments;
        while (c) {
            if (c->post_id == arr[i]->id) stack[stack_count++] = c;
            c = c->next;
        }
        while (stack_count > 0) {
            Comment *cc = stack[--stack_count];
            printf("  - Comment from User %d: %s\n", cc->user_id, cc->text);
        }
    }
    printf("\n============================================================\n");
    free(arr);
}

// [Linked List] Like post
void like_post(AppState *app) {
    int pid;
    printf("Enter post ID to like: ");
    scanf("%d", &pid);
    Post *p = app->posts;
    while (p) {
        if (p->id == pid) {
            // Cek apakah user sudah like
            LikeNode *ln = p->likeList;
            while (ln) {
                if (ln->user_id == app->current_user_id) {
                    printf("Anda sudah like post ini.\n");
                    return;
                }
                ln = ln->next;
            }
            // Tambah like
            LikeNode *newLike = (LikeNode*)malloc(sizeof(LikeNode));
            newLike->user_id = app->current_user_id;
            newLike->next = p->likeList;
            p->likeList = newLike;
            p->likes++;
            save_posts(app); // (opsional: simpan likeList ke file jika ingin persistent)
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

// [Linked List] Unlike post
void unlike_post(AppState *app) {
    int pid;
    printf("\nEnter post ID to unlike: ");
    scanf("%d", &pid);
    Post *p = app->posts;
    while (p) {
        if (p->id == pid) {
            LikeNode **ln = &p->likeList;
            while (*ln) {
                if ((*ln)->user_id == app->current_user_id) {
                    // Hapus LikeNode milik user ini
                    LikeNode *del = *ln;
                    *ln = del->next;
                    free(del);
                    p->likes--;
                    save_posts(app);
                    printf("Post unliked!\n");
                    char notif[MAX_STRING * 2];
                    snprintf(notif, sizeof(notif), "You unliked post ID %d", p->id);
                    enqueueNotif(app, notif);
                    return;
                }
                ln = &(*ln)->next;
            }
            // Jika tidak ditemukan LikeNode user ini
            printf("Anda belum like post ini.\n");
            return;
        }
        p = p->next;
    }
    printf("Post not found.\n");
}

// [Linked List] Comment post
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

// [Linked List & Stack] Delete post (dari linked list, push ke undo stack)
void delete_post(AppState *app) {
    int pid;
    printf("Enter post ID to delete: ");
    scanf("%d", &pid);

    // Hapus dari linked list
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

// [Linked List] Edit post
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

// [Heap] Tampilkan top 3 post by likes
void sort_and_show_posts_by_likes(AppState *app) {
    if (!app->posts) {
        printf("No posts.\n");
        return;
    }
    PostHeap heap;
    build_post_heap(&heap, app->posts);
    printf("Top 3 Posts by Likes:\n");
    for (int i = 0; i < 3 && heap.size > 0; i++) {
        Post *p = extract_max(&heap);
        printf("[%d] User %d: %s (%s) Likes: %d\n", p->id, p->user_id, p->content, p->media, p->likes);
    }
    free_post_heap(&heap);
}

// [Stack] Undo delete post
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

// [BST] Search post by ID (menggunakan BST)
void search_post_by_id(AppState *app) {
    if (!app->posts) {
        printf("No posts.\n");
        return;
    }
    int id;
    printf("Masukkan ID post yang dicari: ");
    scanf("%d", &id);

    PostBSTNode *root = build_post_bst(app->posts);
    Post *found = search_post_bst(root, id);
    if (found)
        printf("Ditemukan: [%d] %s Likes: %d\n", found->id, found->content, found->likes);
    else
        printf("Post dengan ID %d tidak ditemukan.\n", id);
    free_post_bst(root);
}

// [BST/Linked List] Search post by username/ID
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
        search_post_by_id(app);
    } else {
        printf("Pilihan tidak valid.\n");
    }
}

// [Menu] Menu user setelah login
void user_menu(AppState *app); // Function prototype

// [Menu] Menu utama aplikasi
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

// [Menu] Menu user setelah login
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
        printf(" 12.  Log Out\n");
        printf("-----------------------------------------------------\n");
        printf("Pilih menu (1-12): ");
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
            case 12: return;
            default: printf(">> Pilihan tidak valid!\n");
        }
    } while (1);
}

void bubble_sort_posts_by_id(Post **arr, int n) {
    for (int i = 0; i < n-1; i++)
        for (int j = 0; j < n-i-1; j++)
            if (arr[j]->id > arr[j+1]->id) {
                Post *tmp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = tmp;
            }
}

void quick_sort_posts(Post **arr, int left, int right) {
    if (left >= right) return;
    int i = left, j = right;
    int pivot = arr[(left + right) / 2]->likes;
    while (i <= j) {
        while (arr[i]->likes > pivot) i++;
        while (arr[j]->likes < pivot) j--;
        if (i <= j) {
            Post *tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
            i++; j--;
        }
    }
    if (left < j) quick_sort_posts(arr, left, j);
    if (i < right) quick_sort_posts(arr, i, right);
}

// [Searching: Binary Search] Binary search array post by ID
int binary_search_post(Post **arr, int n, int id) {
    int left = 0, right = n - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (arr[mid]->id == id) return mid;
        if (arr[mid]->id < id) left = mid + 1;
        else right = mid - 1;
    }
    return -1;
}

// Tambahkan struct BST untuk User
/* Duplicate definition of UserBSTNode removed */

// [BST] Insert user ke BST User
UserBSTNode* insert_user_bst(UserBSTNode *root, User *user) {
    if (!root) {
        UserBSTNode *node = (UserBSTNode*)malloc(sizeof(UserBSTNode));
        node->user = user;
        node->left = node->right = NULL;
        return node;
    }
    if (strcmp(user->username, root->user->username) < 0)
        root->left = insert_user_bst(root->left, user);
    else if (strcmp(user->username, root->user->username) > 0)
        root->right = insert_user_bst(root->right, user);
    return root;
}

// [BST] Cari user pada BST User berdasarkan username
User* search_user_bst(UserBSTNode *root, const char *username) {
    if (!root) return NULL;
    int cmp = strcmp(username, root->user->username);
    if (cmp == 0) return root->user;
    if (cmp < 0) return search_user_bst(root->left, username);
    else return search_user_bst(root->right, username);
}

// Tambahkan struct BST untuk Comment

// [BST] Insert comment ke BST Comment
CommentBSTNode* insert_comment_bst(CommentBSTNode *root, Comment *comment) {
    if (!root) {
        CommentBSTNode *node = (CommentBSTNode*)malloc(sizeof(CommentBSTNode));
        node->comment = comment;
        node->left = node->right = NULL;
        return node;
    }
    if (comment->id < root->comment->id)
        root->left = insert_comment_bst(root->left, comment);
    else if (comment->id > root->comment->id)
        root->right = insert_comment_bst(root->right, comment);
    return root;
}

// [BST] Cari comment pada BST Comment berdasarkan ID
Comment* search_comment_bst(CommentBSTNode *root, int id) {
    if (!root) return NULL;
    if (id == root->comment->id) return root->comment;
    if (id < root->comment->id) return search_comment_bst(root->left, id);
    else return search_comment_bst(root->right, id);
}

// [Heap berdasarkan jumlah post user
typedef struct {
    User **arr;
    int size;
    int capacity;
} UserHeap;

// Implementasi heapify dan build heap mirip PostHeap, tapi berdasarkan jumlah post user

// [Linked List] Free semua alokasi memori
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

// [Main] Entry point aplikasi
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

    User *u = app.users;
    app.userBST = NULL;
    while (u) {
        app.userBST = insert_user_bst(app.userBST, u);
        u = u->next;
    }

    Comment *c = app.comments;
    app.commentBST = NULL;
    while (c) {
        app.commentBST = insert_comment_bst(app.commentBST, c);
        c = c->next;
    }

    main_menu(&app);
    free_all(&app);
    return 0;
}