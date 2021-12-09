builder:
	clang++ builder.cpp -o builder

create_kernal:
	./builder ${INPUT_FILE}

kernal: builder create_kernal

kernal_test:
	clang++ -g kernal_test.cpp kernal.cpp -o kernal_test

naive:
	clang++ -g naive.cpp -o naive

testing: kernal_test naive

clean:
	rm -f builder host kernal_test naive
