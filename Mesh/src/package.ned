package mesh;

@license(LGPL);

network Mesh
{
    parameters:
        int N = default(5);
    	
    submodules:
	    orchesterator: Orchestrator;
        nodes[N]: Node;
      
     connections:
         for i=0..sizeof(nodes)-1, for j=i+1..sizeof(nodes)-1 {
         		nodes[i].outs++ --> nodes[j].ins++;
         		nodes[i].ins++ <-- nodes[j].outs++;
         }
         
         
         for i=0..sizeof(nodes)-1 {
         	nodes[i].outs++ --> orchesterator.ins++;
         	nodes[i].ins++ <-- orchesterator.outs++;
         }
         
           
}
