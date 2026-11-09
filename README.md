Instructions for using GUI:

Run 'CrimeDatabaseGUI' first

"Crime Records" tab:
To search through the crime records, type in any combination of Crime ID, (Crime) description, and date. Then, click the "SEARCH" button to filter the records down to only matching records.
To insert, fill in all three fields and then click "INSERT".
To delete, fill in only the ID field and click "DELETE".

The search function uses the currently selected tree in the "Tree Comparison" tab. Both the insert and delete functions affect both trees.

"Tree Comparison" tab:
Toggle between the B and B+ trees for searching at the top.
Enter a crime ID in the appropriate field and click "Compare Search" to compare how long searching for the record took between the two trees.
The "Run Benchmarks" button can be clicked at any time to compare the functions of insert, delete, and search between the two trees over thousands of tests in ns per operation.
