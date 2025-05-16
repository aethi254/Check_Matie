n=int(input())
arr=list(map(int,input().split()))
def bestpick(arr,left,right,turn):
    if left>right:
        return 0
    if turn:
        pickright=arr[right]+bestpick(arr,left,right-1,False)
        pickleft=arr[left]+bestpick(arr,left+1,right,False)
        return max(pickleft,pickright)
    if not turn:
        pickright2=bestpick(arr,left,right-1,True)-arr[right]
        pickleft2=bestpick(arr,left+1,right,True)-arr[left]
        return min(pickleft2,pickright2)
if __name__=="__main__":
    diff=bestpick(arr,0,n-1,True)
    if diff>0:
        print("Player 1")
    elif diff<0:
        print("Player 2")
    else:
        print("Draw")
