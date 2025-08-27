#include <iostream>
#include <cassert>
#define MAX_KEYS 5


struct Node
{
    int id;

    int keys[MAX_KEYS];
    uint16_t current_key_count = 0;
    bool is_leaf = true;

    Node* parent = nullptr;

};

struct internal_node : Node
{


    Node* children[MAX_KEYS + 1];
     

}; 
struct leaf_node : Node 
{ 
    int values[MAX_KEYS]; 
    leaf_node *next_leaf; 
}; 

struct insert_up_data
{
    int key;
    Node* left_child;
    Node* right_child;
};



class BtreePlus { 
public: 
    BtreePlus() 
    { 
             
    }
    Node* get_next_node_pointer(int to_insert, internal_node *node)
    {
        for (uint16_t i = 0; i < node->current_key_count; i++)
        {
            if(to_insert < node->keys[i])
            {
                return node->children[i];        //case if found key bigger, then fitting spot
            }
        }
        //case if no key bigger is found, then biggest key, return very last child
        return node->children[node->current_key_count];
        
    }
    int find_left_node_child_index(Node *node)
    {

        internal_node *parent = static_cast<internal_node*>(node->parent); 

        for (int i = 0; i < MAX_KEYS +1; i++)
        {
            if(parent->children[i] == node)
            {
                return i;
            }
        }

        return -1;
        
    }

    void insert(int to_insert)
    {
        Node* cursor = root_node;

        if(!root_node)                  //case first node
        {
            Node *new_node = new leaf_node(); 
            root_node = new_node; 
            root_node->keys[0] = to_insert;
            root_node->current_key_count = 1;   
            return;
        }
        

        while(!cursor->is_leaf)
        {
            internal_node *cursor_cast = static_cast<internal_node*>(cursor);
            cursor = get_next_node_pointer(to_insert,cursor_cast);
        }

        assert(cursor->is_leaf);

        insert_up_data data = {to_insert, nullptr, nullptr};
        insert_key_into_node(data,cursor);


        if(cursor->current_key_count == MAX_KEYS)
        {
            split_leaf(cursor);
        }


    }

    void insert_up_into(insert_up_data data,Node* node)
    {
        insert_key_into_node(data,node);

        if(node->current_key_count == MAX_KEYS)
        {
            split_internal(node);
        }
    }

    void split_leaf(Node* node)
    {
        Node* right_node = new leaf_node();
        
        assert(node->is_leaf);
        leaf_node* node_cast = static_cast<leaf_node*>(node);
        leaf_node* right_node_cast = static_cast<leaf_node*>(right_node);
        right_node_cast->next_leaf = node_cast->next_leaf;
        node_cast->next_leaf = right_node_cast;
       

        int middle_index = MAX_KEYS/2;
        int middle_key = node->keys[middle_index];

        right_node->parent = node->parent;


        int temp_keys[MAX_KEYS];
        std::copy(std::begin(node->keys),std::end(node->keys),std::begin(temp_keys));
        
        std::fill(node->keys + middle_index,node->keys + MAX_KEYS,0);  //fill right side of left node with 0

        std::copy(temp_keys+middle_index, std::end(temp_keys), right_node->keys); //fill left side of right node with left data

        node->current_key_count = middle_index;
        right_node->current_key_count = MAX_KEYS - middle_index;

        if (!node->parent)
        {
            Node* new_parent = new internal_node();
            internal_node* new_parent_cast = static_cast<internal_node*>(new_parent);
            new_parent->is_leaf = false;
            new_parent_cast->children[0] = node;
            new_parent_cast->children[1] = right_node;

            node->parent = new_parent;
            right_node->parent = new_parent;

            root_node = new_parent;

            new_parent->keys[0] = middle_key;
            new_parent->current_key_count = 1;            
        }
        else
        {
            insert_up_data data = {middle_key, node,right_node};

            assert(!node->parent->is_leaf);
            insert_up_into(data,node->parent);
        }
    }

    void split_internal(Node* node)
    {
        Node* right_node = new internal_node();

        right_node->is_leaf = false;
        right_node->parent = node->parent;

        int middle_index = MAX_KEYS/2;
        int middle_key = node->keys[middle_index];

        int temp_keys[MAX_KEYS];
        std::copy(std::begin(node->keys), std::end(node->keys), std::begin(temp_keys));

        std::fill(node->keys + middle_index, node->keys + MAX_KEYS, 0); // zero out right half

        // fill right node with keys greater than middle
        std::copy(temp_keys + middle_index + 1, std::end(temp_keys), right_node->keys);

        node->current_key_count = middle_index;
        right_node->current_key_count = MAX_KEYS - middle_index - 1;

        // handle children pointers
        internal_node* node_cast = static_cast<internal_node*>(node);
        internal_node* right_cast = static_cast<internal_node*>(right_node);

        // left node keeps first (middle_index + 1) children
        // right node gets remaining children
        for (int i = middle_index + 1; i <= MAX_KEYS; i++) {
            right_cast->children[i - (middle_index + 1)] = node_cast->children[i];
            if (node_cast->children[i]) {  // FIXED: Update parent pointer
                node_cast->children[i]->parent = right_node;
            }
            node_cast->children[i] = nullptr;
        }

        if (node->parent)
        {


            insert_up_data data = {middle_key,node,right_node};
            insert_up_into(data,node->parent);
        
        }
        else
        {
            Node* new_parent = new internal_node();
            root_node = new_parent;
            internal_node* new_parent_cast = static_cast<internal_node*>(new_parent);
            new_parent_cast->children[1] = right_node;
            new_parent_cast->children[0] = node;
            new_parent->is_leaf = false;
            new_parent->current_key_count = 1;

            node->parent = new_parent;
            right_node->parent = new_parent;
            
            new_parent->keys[0] = middle_key;
        }

    }


    leaf_node* find_leftmost_leaf(Node* root) {
        if (!root) return nullptr;
        Node* curr = root;

        while (!curr->is_leaf) {
            internal_node* in = static_cast<internal_node*>(curr);
            curr = in->children[0]; // go all the way left
        }
        return static_cast<leaf_node*>(curr);
    }


    void insert_key_into_node(insert_up_data data, Node* node)
    {
        int insert_positon = 0;

        while (insert_positon < node->current_key_count && node->keys[insert_positon] < data.key) {
            insert_positon++;
        }
        
        // Shift elements to the right to make space
        for (int i = node->current_key_count; i > insert_positon; i--) {
            node->keys[i] = node->keys[i - 1];
        }
        //if is an internal node, shift children right too
        if(!node->is_leaf)
        {               
            internal_node *node_cast = static_cast<internal_node*>(node);                                                                    //insert 3, z, i    //position = 2
            for (int i = node->current_key_count+1; i > insert_positon; i--) 
            {                                                                                     //keys[1,2,3,4,0,0] //children[x,y,z,t,0,0] // x y z   t 0 i = 3
                node_cast->children[i] = node_cast->children[i - 1];
            }
            node_cast->children[insert_positon + 1] = data.right_child;
        } 
        // Insert the new key
        node->keys[insert_positon] = data.key;
        
        node->current_key_count++;
    }



    Node* root_node;
    int degree = 3;
};


int main()
{
    BtreePlus myTree;

    while (true)
    {
        std::string x;
        std::cin >> x;

        myTree.insert(std::stoi(x));


        //myTree.print_tree(myTree.root_node,10);
        leaf_node* leaf =  myTree.find_leftmost_leaf(myTree.root_node);

        while (leaf)
        {
            std::cout << "\n leaves: ";
            for(auto key : leaf->keys) 
            {
                std::cout << key << " ";
            }
            leaf = leaf->next_leaf;
        }
        std::cout << "leaves done \n";
        if (myTree.root_node && !myTree.root_node->is_leaf) 
        {
            internal_node* in = static_cast<internal_node*>(myTree.root_node);
            std::cout << "\nRoot keys: ";
            for (int i = 0; i < in->current_key_count; i++) {
                std::cout << in->keys[i] << " ";
            }

            std::cout << "\nChildren: ";
            for (int i = 0; i <= in->current_key_count; i++) {  // note: #children = keys+1
                if (in->children[i]) {
                    std::cout << "(" << i << ")->" << in->children[i]->keys[0] << " ";
                }
            }
            std::cout << "\n";
        }
        
    }
    return 0;
}