#include <iostream>
#include <cassert>
#define MAX_KEYS 3


struct Node
{
    int id;

    int keys[MAX_KEYS];
    uint16_t current_key_count;
    bool is_leaf = true;

    Node* parent = nullptr;

    void insert_into_keys(int x)
    {
        
        // Find insertion position
        int insert_pos = 0;
        while (insert_pos < current_key_count && keys[insert_pos] < x) {
            insert_pos++;
        }
        
        // Shift elements to the right to make space
        for (int i = current_key_count; i > insert_pos; i--) {
            keys[i] = keys[i - 1];
        }
        
        // Insert the new key
        keys[insert_pos] = x;
        current_key_count++;
    }
    void print()
    {
        for (int  i = 0; i < MAX_KEYS; i++)
        {
            //std::cout << keys[i] << " ";
        }
        
    }
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


class BtreePlus { 
public: 
    BtreePlus(int first_key) 
    { 
        Node *new_node = new leaf_node(); 
        root_node = new_node; 
        root_node->keys[0] = first_key;
        root_node->current_key_count = 1;      
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
    int split(Node *node)
    {
        //if (node->current_key_count < MAX_KEYS) return 0;

        if(node->is_leaf)
        {

            Node *node_right = new leaf_node();
            int middle_key_index = node->current_key_count/2;
            int middle_key = node->keys[middle_key_index];
            
            node_right->current_key_count = 0;
            for (uint16_t i = middle_key_index; i < MAX_KEYS; i++)  //fill keys array
            {
                node_right->keys[i-middle_key_index] = node->keys[i];
                node->keys[i] = 0;
                node->current_key_count--;
                node_right->current_key_count++;
            }
            leaf_node *node_left_cast = static_cast<leaf_node*>(node);
            leaf_node *node_right_cast = static_cast<leaf_node*>(node_right);

            node_left_cast->next_leaf = node_right_cast;
            
        
            Node* new_parent;
            if(!node->parent)
            {
                new_parent = new internal_node();
                new_parent->is_leaf = false;
                node->parent = new_parent;
                node_right->parent = new_parent;
                new_parent->current_key_count = 0;
                
                internal_node *parent_cast = static_cast<internal_node*>(new_parent);
                parent_cast->children[0] = node;
                parent_cast->children[1] = node_right;

                //assign new root
                if(node == root_node)
                {
                    root_node = node->parent;
                }
            }
            else
            {
                new_parent = node->parent;
                

                internal_node *parent_cast = static_cast<internal_node*>(new_parent);
                

                parent_cast->children[find_left_node_child_index(node) + 1] = node_right;

                if(node == root_node)
                {
                    root_node = new_parent;
                }
                
            }

            insert_up(middle_key,new_parent);

            node->parent->print();
            
            return 1;
        }

        if(!node->is_leaf)
        {
            perror("reached");
            internal_node *node_right = new internal_node();

            int mid_index = node->current_key_count / 2;
            int mid_key = node->keys[mid_index];

            node_right->current_key_count = 0;


            for (uint16_t i = mid_index+1; i < MAX_KEYS; i++)  //fill keys array
            {
                node_right->keys[i-mid_index-1] = node->keys[i];
                node->keys[i] = 0;
                node->current_key_count--;
             
                node_right->current_key_count++;
            }

            internal_node *node_left = static_cast<internal_node*>(node);
            int children_mid = (MAX_KEYS+1) / 2;
            for (uint16_t i = children_mid; i < MAX_KEYS+1; i++)  //reset children
            {
                node_right->children[i-children_mid] = node_left->children[i];
                node_left->children[i] = nullptr;
            }


        }


        return 1;
    }
    void insert_up(int to_insert, Node *node)
    {
        if(!node->is_leaf)
        {
            internal_node *internal = static_cast<internal_node*>(node);     
            node->insert_into_keys(to_insert);



            if(node->current_key_count == MAX_KEYS)
            {
                perror("split internal");
                split(node);
            }
        }
    }
    void insert(int to_insert, Node *node)
    {
        //case internal, 
        if(!node->is_leaf)
        {
            internal_node *internal = static_cast<internal_node*>(node);     
            Node* to_pass = get_next_node_pointer(to_insert,internal);

            insert(to_insert,to_pass);

            //loop thru internal nodes

        }
        if(node->is_leaf)
        {
            assert(node->current_key_count < MAX_KEYS);
            node->insert_into_keys(to_insert);

            if(node->current_key_count == MAX_KEYS)
            {
                //perform split here
                
                split(node);
            }
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

    Node* root_node;
    int degree = 3;
};


int main()
{
    BtreePlus myTree(30);

    while (true)
    {
        std::string x;
        std::cin >> x;

        myTree.insert(std::stoi(x),myTree.root_node);


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
        
    }
    return 0;
}